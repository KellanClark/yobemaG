#ifndef GAMEBOY_HPP
#define GAMEBOY_HPP

#include <cstring>
#include <iostream>
#include <iterator>
#include <filesystem>
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

#include "apu.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "cartridge.hpp"
#include "timer.hpp"
#include "ram.hpp"

enum systemType : int {
	SYSTEM_DEFAULT,
	SYSTEM_DMG,
	SYSTEM_CGB
};

enum dmaStates : int {
	NO_DMA,
	DMA_IN_PROGRESS,
	WAITING_FOR_DMA
};

class Gameboy {
public:
	GameboyCartridge rom;
	GameboyCPU cpu;
	GameboyPPU ppu;
	GameboyRAM ram;
	GameboyTimer timer;
	GameboyAPU apu;

	Gameboy(void (*joypadWrite_)(uint8_t), uint8_t (*joypadRead_)(),
			void (*ppuVblank)(),
			void (*apuSampleBufferFull)());
	void reset();
	void cycleSystem();

	// Memory functions
	void write8(uint16_t address, uint8_t value);
	void write16(uint16_t address, uint16_t value);
	uint8_t read8(uint16_t address);
	uint16_t read16(uint16_t address);
	void push8(uint8_t value);
	void push16(uint16_t value);
	uint8_t pop8();
	uint16_t pop16();

	// Callback functions because some things rely too much on the platform
	void (*joypadWrite)(uint8_t value);
	uint8_t (*joypadRead)();

	systemType system;
	dmaStates oamDMAState;
	int dmaCyclesLeft;
	// Undocumented R/W CGB registers
	uint8_t undocumented1;	// 0xFF72
	uint8_t undocumented2;	// 0xFF73
	uint8_t undocumented3;	// 0xFF74
	uint8_t undocumented4;	// 0xFF75

private:
};

#endif
