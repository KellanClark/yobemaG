#ifndef GBCPU_HPP
#define GBCPU_HPP

#include "sm83core.hpp"

enum interruptVectors {
	INT_VBLANK = 0x40,
	INT_LCD	   = 0x48,
	INT_TIMER  = 0x50,
	INT_SERIAL = 0x58,
	INT_JOYPAD = 0x60
};

class GameboyCPU : public SM83 {
public:
	GameboyCPU(Gameboy& bus_);

	void reset();
	int cycle();
	void write(uint16_t address, uint8_t value);
	uint8_t read(uint16_t address);
	void requestInterrupt(int intType);

	bool cycleAccurate;
	bool stopped;
	bool halted;
	int counter;
	uint8_t opcode;
	uint8_t highRam[128];
	uint8_t interruptRequests;
	uint8_t enabledInterrupts;
	bool interruptMasterEnable;

private:
};

#endif
