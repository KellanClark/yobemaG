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

class Gameboy {
public:
	GameboyCartridge rom;
	GameboyCPU cpu;
	GameboyPPU ppu;
	GameboyRAM ram;
	GameboyTimer timer;
	GameboyAPU apu;

	Gameboy(void (*joypadWrite_)(uint8_t), uint8_t (*joypadRead_)(), void (*serialWrite_)(uint16_t, uint8_t), uint8_t (*serialRead_)(uint16_t));
	void cycleCpu();
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
	void (*serialWrite)(uint16_t address, uint8_t value);
	uint8_t (*serialRead)(uint16_t address);

	bool cgbMode;
	int dmaCountdown; // 1 = Ready 2 = Waiting to finnish instruction
	int dmaCyclesLeft;
	// Undocumented R/W CGB registers
	uint8_t undocumented1;	// 0xFF02
	uint8_t undocumented2;	// 0xFF03
	uint8_t undocumented3;	// 0xFF04
	uint8_t undocumented4;	// 0xFF05

private:
};

#endif
