#ifndef GBPPU_HPP
#define GBPPU_HPP

enum ppuModes {
	PPU_HBLANK = 0,
	PPU_VBLANK = 1,
	PPU_OAM_TRANSFER = 2,
	PPU_LCD_TRANSFER = 3
};

class Gameboy;
class GameboyPPU {
public:
	Gameboy& bus;

	GameboyPPU(Gameboy& bus_);
	void cycle();
	void write(uint16_t address, uint8_t value);
	uint8_t read(uint16_t address);
	void setMode(enum ppuModes newMode);
	void checkLCDStatusForInterrupt();

	uint8_t outputFramebuffer[160*144];
	uint8_t vram[0x2000];
	typedef union {
		struct {
			uint8_t yPos;
			uint8_t xPos;
			uint8_t tileIndex;
			struct {
				uint8_t paletteCGB : 3;
				uint8_t tileBank : 1;
				uint8_t paletteDMG : 1;
				uint8_t xFlip : 1;
				uint8_t yFlip : 1;
				uint8_t priority : 1;
			};
		};
		uint32_t raw;
	} OamEntry;
	union {
		OamEntry oamEntries[40];
		uint8_t oam[160];
	};
	
	bool frameDone;
	int column;
	int modeCycle;
	int lineCycle;
	int frameCycle;
	bool oamEnable;
	bool vramEnable;

	// Hardware registers
	typedef union {
		struct {
			uint8_t bgWindowEnable : 1;
			uint8_t objEnable : 1;
			uint8_t objSize : 1;
			uint8_t bgTileMapOffset : 1;
			uint8_t tileDataNotOffset : 1;
			uint8_t windowEnable : 1;
			uint8_t windowTileMapOffset : 1;
			uint8_t lcdEnable : 1;
		};
		uint8_t value;
	} LCDC_t;
	LCDC_t lcdc;			// 0xFF40
	union {
		struct {
			uint8_t mode : 2;
			uint8_t lineCoincidence : 1;
			uint8_t interruptHBlank : 1;
			uint8_t interruptVBlank : 1;
			uint8_t interruptOAM : 1;
			uint8_t interruptLYC : 1;
			uint8_t : 1;
		};
		uint8_t lcdStatus;		// 0xFF41
	};
	uint8_t scrollY;			// 0xFF42
	uint8_t scrollX;			// 0xFF43
	uint8_t line;				// 0xFF44
	uint8_t lineCompare;		// 0xFF45
	bool conditionsMetPrevious;
	uint8_t dmaAddress;			// 0xFF46
	uint8_t backgroundPalette;	// 0xFF47
	uint8_t objectPalette0;		// 0xFF48
	uint8_t objectPalette1;		// 0xFF49
	uint8_t windowY;			// 0xFF4A
	uint8_t windowX;			// 0xFF4B

	// Log values of certain registers throughout a line
	uint8_t bgPalettes[160];
	LCDC_t lcdcValues[160];

	// LCD Transfer variables
	uint8_t scanline[160];
	OamEntry oamBuffer[10];
	int oamBufferSize;
	uint8_t objScanline[160];
	bool objScanlineTransparent[160];
	bool windowTriggeredThisFrame;
	uint8_t windowLineCounter;
};

#endif
