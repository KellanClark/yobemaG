
#include "gameboy.hpp"

GameboyRAM::GameboyRAM(Gameboy& bus_) : bus(bus_) {
	ram = (uint8_t *)calloc(0x2000, sizeof(uint8_t));
}

void GameboyRAM::write(uint16_t address, uint8_t value) {
	// Convert echo ram address to normal address
	if ((address >= 0xE000) && (address <= 0xFDFF))
		address -= 0x2000;

	if ((address >= 0xC000) && (address <= 0xDFFF))
		ram[address - 0xC000] = value;

	return;
}

uint8_t GameboyRAM::read(uint16_t address) {
	// Convert echo ram address to normal address
	if ((address >= 0xE000) && (address <= 0xFDFF))
		address -= 0x2000;

	if ((address >= 0xC000) && (address <= 0xDFFF))
		return ram[address - 0xC000];

	return 0xFF;
}