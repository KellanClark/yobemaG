
#include "gameboy.hpp"

GameboyTimer::GameboyTimer(Gameboy& bus_) : bus(bus_) {
	divider=counter=modulo=control.value=andResultPrevious=interruptCountdown = 0;
	reloadedThisCycle = 0;

	return;
}

void GameboyTimer::cycle() {
	divider += 4;

	if (interruptCountdown) { // Continuation of TIMA overflow after 1 T Cycle
		interruptCountdown = 0;
		counter = modulo;
		bus.cpu.requestInterrupt(INT_TIMER);
		reloadedThisCycle = true;
	} else {
		reloadedThisCycle = false;
	}

	int andResult;
	int frameSequencerBit;
	switch (control.speed) {
	case 0:
		andResult = ((1 << 9) & divider) >> 9;
		break;
	case 1:
		andResult = ((1 << 3) & divider) >> 3;
		break;
	case 2:
		andResult = ((1 << 5) & divider) >> 5;
		break;
	case 3:
		andResult = ((1 << 7) & divider) >> 7;
		break;
	}
	andResult &= control.enable;
	frameSequencerBit = (divider & (1 << 12)) >> 12;

	if (andResultPrevious && !andResult) {
		++counter;
		if (counter == 0) // TIMA overflow
			interruptCountdown = 1;
	}
	if (frameSequencerBitPrevious && !frameSequencerBit) {
		bus.apu.tickFrameSequencer = true;
	}

	andResultPrevious = andResult;
	frameSequencerBitPrevious = frameSequencerBit;

	return;
}

void GameboyTimer::write(uint16_t address, uint8_t value) {
	switch (address) {
	case 0xFF04: // DIV
		divider = 0;
		return;
	case 0xFF05: // TIMA
		if (interruptCountdown)
			interruptCountdown = 0;
		if (!reloadedThisCycle)
			counter = value;
		return;
	case 0xFF06: // TMA
		modulo = value;
		return;
	case 0xFF07: // TAC
		control.value = value & 7;
		return;
	default:
		return;
	}
}

uint8_t GameboyTimer::read(uint16_t address) {
	switch (address) {
	case 0xFF04: // DIV
		return divider >> 8;
	case 0xFF05: // TIMA
		return counter;
	case 0xFF06: // TMA
		return modulo;
	case 0xFF07: // TAC
		return control.value | ~7;
	default:
		return 0xFF;
	}
}