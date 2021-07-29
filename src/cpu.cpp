
#include "../include/gameboy.hpp"

GameboyCPU::GameboyCPU(Gameboy& bus_) : SM83(bus_) {
	reset();
}

void GameboyCPU::reset() {
	memset(highRam, 0, 127); // Clear high ram

	counter=interruptRequests=enabledInterrupts = 0; // Clear variables
	stopped=halted = false;
	interruptMasterEnable = false;
}

int GameboyCPU::cycle() {
	uint8_t readyInterrupts = (interruptRequests & enabledInterrupts);
	if (readyInterrupts) {
		// Temporary HALT implementation
		halted = false;

		// Check and handle interrupt
		if (interruptMasterEnable) {
			interruptMasterEnable = false;
			bus.push16(r.pc);
			if (readyInterrupts & 1) {
				r.pc = INT_VBLANK;
				interruptRequests &= ~1;
			} else if (readyInterrupts & 2) {
				r.pc = INT_LCD;
				interruptRequests &= ~2;
			} else if (readyInterrupts & 4) {
				r.pc = INT_TIMER;
				interruptRequests &= ~4;
			} else if (readyInterrupts & 8) {
				r.pc = INT_SERIAL;
				interruptRequests &= ~8;
			} else if (readyInterrupts & 16) {
				r.pc = INT_JOYPAD;
				interruptRequests &= ~16;
			}
			// Add delay
			bus.cycleSystem();
			bus.cycleSystem();
			bus.cycleSystem();
			bus.cycleSystem();
			bus.cycleSystem();
			return 0;
		}
	}

	// TODO: Deal with stopped and halted states properly
	if (halted || stopped) {
		bus.cycleSystem();
		return 1;
	}

	executeOpcode();

	return 0;
}

void GameboyCPU::write(uint16_t address, uint8_t value) {
	switch (address) {
	case 0xFF0F:
		interruptRequests = value;
		return;
	case 0xFF80 ... 0xFFFE:
		highRam[address & 0x007F] = value;
		return;
	case 0xFFFF:
		enabledInterrupts = value;
		return;
	default:
		return;
	}
}

uint8_t GameboyCPU::read(uint16_t address) {
	switch (address) {
	case 0xFF0F:
		return interruptRequests;
	case 0xFF80 ... 0xFFFE:
		return highRam[address & 0x007F];
	case 0xFFFF:
		return enabledInterrupts;
	default:
		return 0xFF;
	}
}

void GameboyCPU::requestInterrupt(int intType) {
	interruptRequests |= (1 << ((intType-0x40)/8)); // Set the correct interrupt flag

	return;
}
