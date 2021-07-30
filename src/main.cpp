
#include "imgui.h"
#include "backends/imgui_impl_sdl.h"
#include "backends/imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include <GL/gl3w.h>
#include <nfd.hpp>

#include "gameboy.hpp"

// Argument Variables
bool argRomGiven;
std::filesystem::path argRomFilePath;
bool argBootromGiven;
std::filesystem::path argBootromFilePath;
systemType argSystem;
bool argLogConsole;

// I have no idea how this works. I found this piece of magic on the internet somewhere.
constexpr auto cexprHash(const char *str, std::size_t v = 0) noexcept -> std::size_t {
	return (*str == 0) ? v : 31 * cexprHash(str + 1) + *str;
}

// Joypad stuff
union {
	struct {
		uint8_t right : 1;
		uint8_t left : 1;
		uint8_t up : 1;
		uint8_t down : 1;
		uint8_t a : 1;
		uint8_t b : 1;
		uint8_t select : 1;
		uint8_t start : 1;
	};
	uint8_t value;
} joypadButtons;
uint8_t joypadActionSelect;
uint8_t joypadDirectionSelect;
void joypadWriteCallback(uint8_t value);
uint8_t joypadReadCallback();

// Serial stuff
uint8_t serialData;
uint8_t serialControl;
void serialWriteCallback(uint16_t address, uint8_t value);
uint8_t serialReadCallback(uint16_t address);

// Audio stuff
SDL_AudioSpec desiredAudioSpec, audioSpec;
SDL_AudioDeviceID audioDevice;
std::vector<int16_t> wavFileData;
std::ofstream wavFileStream;
void sampleBufferCallback();

Gameboy emulator(&joypadWriteCallback, &joypadReadCallback,
				&serialWriteCallback, &serialReadCallback,
				&sampleBufferCallback);

int loadRomFromFile();

// ImGui Windows
void mainMenuBar();
bool showRomInfo;
void romInfoWindow();

bool var1;
bool var2;
bool var3;

int main(int argc, char *argv[]) {
	// Parse arguments
	argRomGiven = false;
	argBootromGiven = false;
	argBootromFilePath = "";
	argSystem = SYSTEM_DEFAULT;
	argLogConsole = false;
	for (int i = 1; i < argc; i++) {
		switch (cexprHash(argv[i])) {
		case cexprHash("--rom"):
			if (argc == (++i)) {
				printf("Not enough arguments for flag --rom\n");
				return -1;
			}
			argRomGiven = true;
			argRomFilePath = argv[i];
			break;
		case cexprHash("--bootrom"):
			if (argc == ++i ) {
				printf("Not enough arguments for flag --bootrom\n");
				return -1;
			}
			argBootromGiven = true;
			argBootromFilePath = argv[i];
			break;
		case cexprHash("--system"):
			if (argc == ++i) {
				printf("Not enough arguments for flag --system\n");
				return -1;
			}
			switch (cexprHash(argv[i])) {
			case cexprHash("gb"):
			case cexprHash("dmg"):
				argSystem = SYSTEM_DMG;
				break;
			case cexprHash("gbc"):
			case cexprHash("cgb"):
				argSystem = SYSTEM_CGB;
				break;
			default:
				printf("Unknown argument for flag --system:  %s", argv[i]);
				return -1;
			}
			break;
		case cexprHash("--log-to-console"):
			argLogConsole = true;
			break;
		default:
			if (i == 1) {
				argRomGiven = true;
				argRomFilePath = argv[i];
			} else {
				printf("Unknown argument:  %s\n", argv[i]);
				return -1;
			}
		}
	}

	if (argRomGiven) {
		if (loadRomFromFile() == -1) {
			return -1;
		}
	}

	// Initalize some values
	serialData = 0;
	serialControl = 0;
	joypadButtons.value = 0xFF;

	/* Temporary hex viewer
	for (int i = 0; i < 0x400; i += 16) {
		printf("%04X: ", i);
		for (int j = 0; j < 16; j++) {
			printf(" %02X", emulator.rom.romBuff[i+j]);
		}
		printf("\n");
	}*/

	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}
	const char* glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	std::string windowName = "yoBemaG - ";
	windowName += argRomFilePath;
	SDL_Window* window = SDL_CreateWindow(windowName.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync
	if (gl3wInit()) {
		printf("Failed to initialize OpenGL loader!\n");
		return 1;
	}

	// Setup Audio
	desiredAudioSpec = {
		.freq = 48000,
		.format = AUDIO_S16,
		.channels = 2,
		.samples = 1024,
		.callback = NULL
	};
	audioDevice = SDL_OpenAudioDevice(NULL, 0, &desiredAudioSpec, &audioSpec, 0);
	SDL_PauseAudioDevice(audioDevice, 0);
	emulator.apu.sampleRate = audioSpec.freq;

	// Setup ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	 // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;	  // Enable Gamepad Controls
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	bool showDemoWindow = true;
	bool showAnotherWindow = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	NFD::Guard nfdGuard; // Setup Native File Dialogs

	// Create image for main display
	GLuint lcdTexture;
	glGenTextures(1, &lcdTexture);
	glBindTexture(GL_TEXTURE_2D, lcdTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // GL_LINEAR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/*uint8_t colors[4][4] = {
		{155, 188,  15, 255},
		{139, 172,  15, 255},
		{ 48,  98,  48, 255},
		{ 15,  56,  15, 255}};*/
	uint32_t colors[4] = {
		(uint32_t)((255 << 24) | (255 << 16) | (255 << 8) | 255),
		(uint32_t)((170 << 24) | (170 << 16) | (170 << 8) | 255),
		(uint32_t)(( 85 << 24) | ( 85 << 16) | ( 85 << 8) | 255),
		(uint32_t)((  0 << 24) | (  0 << 16) | (  0 << 8) | 255)};
	uint32_t pixels[160 * 144];
	memset(pixels, 0, sizeof(pixels));
	memset(emulator.ppu.outputFramebuffer, 0, sizeof(emulator.ppu.outputFramebuffer));

	// Main loop
	bool debug = false;
	bool unlockFramerate = false;
	SDL_Event event;
	bool done = false;
	while (!done) {
		unsigned int frameStartTicks = SDL_GetTicks();

		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);

			bool directionButtonPressed;
			bool actionButtonPressed;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;
			switch (event.type) {
			case SDL_QUIT:
				done = true;
				break;
			case SDL_KEYDOWN:
				directionButtonPressed = false;
				actionButtonPressed = false;
				switch (event.key.keysym.sym) {
				case SDLK_d:
					//debug = true;
					break;
				case SDLK_u:
					unlockFramerate = unlockFramerate ? false : true;
					break;
				case SDLK_s:
					emulator.rom.save();
					break;
				case SDLK_RIGHT:
					joypadButtons.right = 0;
					directionButtonPressed = true;
					break;
				case SDLK_LEFT:
					joypadButtons.left = 0;
					directionButtonPressed = true;
					break;
				case SDLK_UP:
					joypadButtons.up = 0;
					directionButtonPressed = true;
					break;
				case SDLK_DOWN:
					joypadButtons.down = 0;
					directionButtonPressed = true;
					break;
				case SDLK_z:
					joypadButtons.b = 0;
					actionButtonPressed = true;
					break;
				case SDLK_x:
					joypadButtons.a = 0;
					actionButtonPressed = true;
					break;
				case SDLK_n:
					joypadButtons.select = 0;
					actionButtonPressed = true;
					break;
				case SDLK_m:
					joypadButtons.start = 0;
					actionButtonPressed = true;
					break;
				}
				if ((directionButtonPressed && !joypadDirectionSelect) || (actionButtonPressed && !joypadActionSelect))
					emulator.cpu.requestInterrupt(INT_JOYPAD);
				break;
			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
				case SDLK_RIGHT:
					joypadButtons.right = 1;
					break;
				case SDLK_LEFT:
					joypadButtons.left = 1;
					break;
				case SDLK_UP:
					joypadButtons.up = 1;
					break;
				case SDLK_DOWN:
					joypadButtons.down = 1;
					break;
				case SDLK_z:
					joypadButtons.b = 1;
					break;
				case SDLK_x:
					joypadButtons.a = 1;
					break;
				case SDLK_n:
					joypadButtons.select = 1;
					break;
				case SDLK_m:
					joypadButtons.start = 1;
					break;
				}
				break;
			}
		}

		emulator.ppu.frameDone = false;
		while (!emulator.ppu.frameDone && argRomGiven) {
			//if (emulator.cpu.r.pc == 0x0098) debug = true;
			//	printf("0x%04X\n", emulator.cpu.r.pc);
			if (argLogConsole)
				printf("A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: 00:%04X (%02X %02X %02X %02X)\n", emulator.cpu.r.a, emulator.cpu.r.f, emulator.cpu.r.b, emulator.cpu.r.c, emulator.cpu.r.d, emulator.cpu.r.e, emulator.cpu.r.h, emulator.cpu.r.l, emulator.cpu.r.sp, emulator.cpu.r.pc, emulator.read8(emulator.cpu.r.pc), emulator.read8(emulator.cpu.r.pc + 1), emulator.read8(emulator.cpu.r.pc + 2), emulator.read8(emulator.cpu.r.pc + 3));
			if (debug) {
				printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
				printf("AF:  0x%02X %02X\n", emulator.cpu.r.a, emulator.cpu.r.f);
				printf("BC:  0x%02X %02X\n", emulator.cpu.r.b, emulator.cpu.r.c);
				printf("DE:  0x%02X %02X\n", emulator.cpu.r.d, emulator.cpu.r.e);
				printf("HL:  0x%02X %02X\n", emulator.cpu.r.h, emulator.cpu.r.l);
				printf("\n");
				printf("Flags\n");
				printf("Zero:  %d\n", (emulator.cpu.r.f&0x80)>>7);
				printf("Subtract:  %d\n", (emulator.cpu.r.f&0x40)>>6);
				printf("Half Carry:  %d\n", (emulator.cpu.r.f&0x20)>>5);
				printf("Carry:  %d\n", (emulator.cpu.r.f&0x10)>>4);
				printf("\n");
				printf("PC:  0x%X\n", emulator.cpu.r.pc);
				printf("SP:  0x%X\n", emulator.cpu.r.sp);
				printf("IME:  %d\n", emulator.cpu.interruptMasterEnable);
				printf("IF:  %d\n", emulator.cpu.interruptRequests);
				printf("IE:  %d\n", emulator.cpu.enabledInterrupts);
				printf("\n");
				printf("Data at PC:  0x%02X, 0x%02X, 0x%02X, 0x%02X\n", emulator.read8(emulator.cpu.r.pc), emulator.read8(emulator.cpu.r.pc+1), emulator.read8(emulator.cpu.r.pc+2), emulator.read8(emulator.cpu.r.pc+3));
				printf("Data at SP:  0x%02X, 0x%02X, 0x%02X, 0x%02X\n", emulator.read8(emulator.cpu.r.sp), emulator.read8(emulator.cpu.r.sp+1), emulator.read8(emulator.cpu.r.sp+2), emulator.read8(emulator.cpu.r.sp+3));
				printf("\n");
				printf("PPU Mode:  %d\n", emulator.ppu.mode);
				printf("PPU Mode Cycle:  %d\n", emulator.ppu.modeCycle);
				printf("Current Scanline:  %d\n", emulator.ppu.line);
				getchar();
			}
			emulator.cpu.cycle();
		}

		// Temporary audio bandaid
		if (emulator.ppu.lcdc.lcdEnable && emulator.apu.sampleBufferIndex) {
			sampleBufferCallback();
			emulator.apu.sampleBufferIndex = 0;
		}

		// Convert framebuffer to screen colors
		for (int i = 0; i < (160 * 144); i++) {
			pixels[i] = colors[emulator.ppu.outputFramebuffer[i]];
		}
		glBindTexture(GL_TEXTURE_2D, lcdTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 160, 144, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pixels);

		/* Draw ImGui Stuff */
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		if (showDemoWindow)
			ImGui::ShowDemoWindow(&showDemoWindow);

		mainMenuBar();

		if (showRomInfo)
			romInfoWindow();

		// Gameboy Screen
		{
			ImGui::Begin("Gameboy Screen");

			ImGui::Text("Here's some sample text.");
			ImGui::Image((void*)(intptr_t)lcdTexture, ImVec2(160*2, 144*2));

			ImGui::End();
		}

		// Example Window
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");						  // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");			   // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &showDemoWindow);	  // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &showAnotherWindow);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);			// Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))							// Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();

			if (showAnotherWindow) {
				ImGui::Begin("Another Window", &showAnotherWindow);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
				ImGui::Text("Hello from another window!");
				if (ImGui::Button("Close Me"))
					showAnotherWindow = false;
				ImGui::End();
			}
		}

		/* Rendering */
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);

		if (!unlockFramerate && 0) {
			while ((SDL_GetTicks() - frameStartTicks) <= ((1000 / 60) - 2));
				//sleep(1);
		}
	}

	/* Cleanup */

	// WAV file
	struct  __attribute__((__packed__)) {
		char riffStr[4] = {'R', 'I', 'F', 'F'};
		unsigned int fileSize = 0;
		char waveStr[4] = {'W', 'A', 'V', 'E'};
		char fmtStr[4] = {'f', 'm', 't', ' '};
		unsigned int subchunk1Size = 16;
		unsigned short audioFormat = 1;
		unsigned short numChannels = 2;
		unsigned int sampleRate = audioSpec.freq;
		unsigned int byteRate = audioSpec.freq * sizeof(int16_t) * 2;
		unsigned short blockAlign = 4;
		unsigned short bitsPerSample = sizeof(int16_t) * 8;
		char dataStr[4] = {'d', 'a', 't', 'a'};
		unsigned int subchunk2Size = 0;
	} wavHeaderData;
	wavFileStream.open("output.wav", std::ios::binary | std::ios::trunc);
	wavHeaderData.subchunk2Size = (wavFileData.size() * sizeof(int16_t));
	wavHeaderData.fileSize = sizeof(wavHeaderData) - 8 + wavHeaderData.subchunk2Size;
	wavFileStream.write(reinterpret_cast<const char*>(&wavHeaderData), sizeof(wavHeaderData));
	wavFileStream.write(reinterpret_cast<const char*>(wavFileData.data()), wavFileData.size() * sizeof(int16_t));
	wavFileStream.close();

	// ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	// SDL
	SDL_CloseAudioDevice(audioDevice);
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

void joypadWriteCallback(uint8_t value) {
	joypadDirectionSelect = value & 0x10;
	joypadActionSelect = value & 0x20;
}

uint8_t joypadReadCallback() {
	uint8_t returnValue = 0;
	returnValue = 0xC0 | joypadActionSelect | joypadDirectionSelect;
	if (!joypadDirectionSelect)
		returnValue |= joypadButtons.value & 0xF;
	if (!joypadActionSelect)
		returnValue |= joypadButtons.value >> 4;

	return returnValue;
}

// Temporary serial implementation for running blargg's tests
void serialWriteCallback(uint16_t address, uint8_t value) {
	return;

	if (address == 0xFF01) {
		serialData = value;
		putchar(serialData);
	}

	if (address == 0xFF02)
		serialControl = value;
}

uint8_t serialReadCallback(uint16_t address) {
	return 0xFF;

	if (address == 0xFF01)
		return serialData;

	if (address == 0xFF02)
		return serialControl;

	return 0xFF;
}

void sampleBufferCallback() {
	wavFileData.insert(wavFileData.end(), emulator.apu.sampleBuffer.begin(), emulator.apu.sampleBuffer.begin() + emulator.apu.sampleBufferIndex);
	SDL_QueueAudio(audioDevice, &emulator.apu.sampleBuffer, emulator.apu.sampleBufferIndex * sizeof(int16_t));
}

int loadRomFromFile() {
	emulator.reset();

	if (emulator.rom.load(argRomFilePath, argBootromFilePath, argSystem))
		return -1;

	return 0;
}

void mainMenuBar() {
	ImGui::BeginMainMenuBar();

	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("Load ROM")) {
			nfdfilteritem_t filter[1] = {{"Game Boy/Game Boy Color ROM", "gb,gbc,bin"}};
			NFD::UniquePath nfdRomFilePath;
			nfdresult_t nfdResult = NFD::OpenDialog(nfdRomFilePath, filter, 1);
			if (nfdResult == NFD_OKAY) {
				printf("Selected file: %s\n", nfdRomFilePath.get());
				argRomGiven = true;
				argRomFilePath = nfdRomFilePath.get();
				loadRomFromFile();
			} else if (nfdResult == NFD_CANCEL) {
				printf("No file selected.\n");
			} else {
				printf("Error: %s\n", NFD::GetError());
			}
		}
		ImGui::Separator();
		ImGui::MenuItem("ROM Info", NULL, &showRomInfo, argRomGiven);

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Menu 1")) {
		ImGui::MenuItem("Test 1", NULL, &var1);
		ImGui::MenuItem("Test 2", NULL, &var2);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Menu 2")) {
		ImGui::MenuItem("Test 3", NULL, &var3);
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

void romInfoWindow() {
	ImGui::Begin("ROM Info", &showRomInfo);

	ImGui::Text("Rom File Name:  %s\n", argRomFilePath.c_str());
	ImGui::Text("Bootrom File Name:  %s\n", argBootromFilePath.c_str());
	ImGui::Text("File Size:  %d\n", (int)emulator.rom.romBuff.size());
	ImGui::Text("Rom Name:  %s\n", emulator.rom.name.c_str());
	ImGui::Text("External ROM Banks:  %d\n", emulator.rom.extROMBanks);
	ImGui::Text("External ROM Size:  %d\n", emulator.rom.extROMBanks * 16 * 1024);
	ImGui::Text("Game Supports DMG:  %s\n", (emulator.rom.dmgSupported ? "True" : "False"));
	ImGui::Text("Game Supports CGB:  %s\n", (emulator.rom.dmgSupported ? "True" : "False"));
	ImGui::Text("Game Supports SGB Features:  %s\n", (emulator.rom.sgbSupported ? "True" : "False"));
	ImGui::Text("Memory Bank Controller:  %s\n", emulator.rom.mbcString);
	ImGui::Text("Game Has Save Battery:  %s\n", (emulator.rom.saveBatteryEnabled ? "True" : "False"));
	ImGui::Text("Game Has Real Time Clock:  %s\n", (emulator.rom.rtcEnabled ? "True" : "False"));
	ImGui::Text("External RAM Banks:  %d\n", emulator.rom.extRAMBanks);
	ImGui::Text("External RAM Size:  %d\n", emulator.rom.extRAMBanks * 8 * 1024);

	ImGui::End();
}