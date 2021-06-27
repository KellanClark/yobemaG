
#include "gameboy.hpp"

GameboyAPU::GameboyAPU(Gameboy& bus_) : bus(bus_) {
	//

	return;
}

void GameboyAPU::cycle() {
	//

	return;
}

void GameboyAPU::write(uint16_t address, uint8_t value) {
	switch (address) {
	default:
		return;
	}
}

uint8_t GameboyAPU::read(uint16_t address) {
	switch (address) {
	default:
		return 0xFF;
	}
}