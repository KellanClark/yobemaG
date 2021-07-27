
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include <SDL2/SDL.h>
#include "glad/glad.h"

#include "gameboy.hpp"

#define SCREEN_WIDTH 160*4
#define SCREEN_HEIGHT 144*4

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

int main(int argc, char *argv[]) {
	// Parse arguments
	if (argc < 2) {
		printf("Not enough arguments.\n");
		return -1;
	}
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
			if (argc == (++i)) {
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
			case cexprHash("dmg"):
				argSystem = SYSTEM_DMG;
				break;
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
	if (!argRomGiven) {
		printf("No ROM given.");
		return -1;
	}

	// Load cartridge
	if (argBootromGiven)
		printf("Bootrom File Name:  %s\n", argBootromFilePath.c_str());
	if (emulator.rom.load(argRomFilePath, argBootromFilePath, argSystem))
		return -1;

	// Print info about cartridge
	printf("Rom File Name:  %s\n", argRomFilePath.c_str());
	printf("File Size:  %d\n", (int)emulator.rom.romBuff.size());
	printf("Rom Name:  %s\n", emulator.rom.name.c_str());
	printf("External ROM Banks:  %d\n", emulator.rom.extROMBanks);
	printf("External ROM Size:  %d\n", emulator.rom.extROMBanks * 16 * 1024);
	printf("Game Supports DMG:  %s\n", (emulator.rom.dmgSupported ? "True" : "False"));
	printf("Game Supports CGB:  %s\n", (emulator.rom.dmgSupported ? "True" : "False"));
	printf("Game Supports SGB Features:  %s\n", (emulator.rom.sgbSupported ? "True" : "False"));
	printf("Memory Bank Controller:  %s\n", emulator.rom.mbcString);
	printf("Game Has Save Battery:  %s\n", (emulator.rom.saveBatteryEnabled ? "True" : "False"));
	printf("Game Has Real Time Clock:  %s\n", (emulator.rom.rtcEnabled ? "True" : "False"));
	printf("External RAM Banks:  %d\n", emulator.rom.extRAMBanks);
	printf("External RAM Size:  %d\n", emulator.rom.extRAMBanks * 8 * 1024);

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

	// Start SDL
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	std::string windowName = "yobemaG - ";
	windowName += emulator.rom.name.c_str();
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window *window = SDL_CreateWindow(windowName.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, windowFlags);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
	gladLoadGL();
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
	// Audio
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
	// Create texture for drawing to screen
	SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STATIC, 160, 144);

	// WAV file
	struct  __attribute__((__packed__)) {
		char riffStr[4] = {'R', 'I', 'F', 'F'};
		unsigned int fileSize = 0;
		char waveStr[4] = {'W', 'A', 'V', 'E'};
		char fmtStr[4] = {'f', 'm', 't', ' '};
		unsigned int subchunk1Size = 16;
		unsigned short audioFormat = 1;
		unsigned short numChannels = 2;
		unsigned int sampleRate = 48000;
		unsigned int byteRate = 48000 * 2 * 2;
		unsigned short blockAlign = 4;
		unsigned short bitsPerSample = 16;
		char dataStr[4] = {'d', 'a', 't', 'a'};
		unsigned int subchunk2Size = 0;
	} wavHeaderData;
	wavFileStream.open("output.wav", std::ios::binary | std::ios::trunc);

	/*SDL_Color colors[4] = {
		{155, 188,  15, 255},
		{139, 172,  15, 255},
		{ 48,  98,  48, 255},
		{ 15,  56,  15, 255}};*/
	SDL_Color colors[4] = {
		{255, 255, 255, 255},
		{170, 170, 170, 255},
		{ 85,  85,  85, 255},
		{  0,   0,   0, 255}};
	uint32_t pixels[160 * 144];
	memset(pixels, 0, sizeof(pixels));
	memset(emulator.ppu.outputFramebuffer, 0, 160 * 144 * sizeof(uint8_t));

	bool debug = false;
	bool unlockFramerate = false;
	bool quit = false;
	SDL_Event event;
	while (!quit) {
		unsigned int frameStartTicks = SDL_GetTicks();

		// Handle keypresses and other events
		while(SDL_PollEvent(&event) != 0) {
			bool directionButtonPressed;
			bool actionButtonPressed;
			switch (event.type) {
			case SDL_QUIT:
				quit = true;
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
		while (!emulator.ppu.frameDone) {
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
		if (emulator.ppu.lcdc.lcdEnable) {
			sampleBufferCallback();
			emulator.apu.sampleBufferIndex = 0;
		}

		// Convert framebuffer to screen colors
		for (int i = 0; i < (160 * 144); i++) {
			uint8_t color = emulator.ppu.outputFramebuffer[i];
			pixels[i] = (colors[color].r << 24) | (colors[color].g << 16) | (colors[color].b << 8);
		}

		// Draw everything on the screen
		SDL_UpdateTexture(texture, NULL, pixels, 160 * sizeof(uint32_t));
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		if (!unlockFramerate) {
			while ((SDL_GetTicks() - frameStartTicks) < (1000 / 60));
				//sleep(1);
		}
	}

	wavHeaderData.subchunk2Size = (wavFileData.size() * sizeof(int16_t));
	wavHeaderData.fileSize = sizeof(wavHeaderData) - 8 + wavHeaderData.subchunk2Size;
	wavFileStream.write(reinterpret_cast<const char*>(&wavHeaderData), sizeof(wavHeaderData));
	wavFileStream.write(reinterpret_cast<const char*>(wavFileData.data()), wavFileData.size() * sizeof(int16_t));
	wavFileStream.close();

	SDL_CloseAudioDevice(audioDevice);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

void joypadWriteCallback(uint8_t value) {
	joypadDirectionSelect = value & 0x10;
	joypadActionSelect = value & 0x20;

	return;
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
	//return;

	if (address == 0xFF01) {
		serialData = value;
		putchar(serialData);
	}

	if (address == 0xFF02)
		serialControl = value;

	return;
}

uint8_t serialReadCallback(uint16_t address) {
	//return 0xFF;

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