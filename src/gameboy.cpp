
#include "gameboy.hpp"

Gameboy::Gameboy(void (*joypadWrite_)(uint8_t), uint8_t (*joypadRead_)(), void (*serialWrite_)(uint16_t, uint8_t), uint8_t (*serialRead_)(uint16_t)) : rom(*this), cpu(*this), ppu(*this), ram(*this), timer(*this), apu(*this), joypadWrite(joypadWrite_), joypadRead(joypadRead_), serialWrite(serialWrite_), serialRead(serialRead_) {
	dmaCyclesLeft=undocumented1=undocumented2=undocumented3=undocumented4 = 0;
}

void Gameboy::cycleSystem() {
	timer.cycle();
	for (int i = 0; i < 4; i++) {
		apu.cycle(); // Not implemented
		ppu.cycle();
	}
	if (dmaCyclesLeft)
		--dmaCyclesLeft;
	if (oamDMAState == WAITING_FOR_DMA)
		oamDMAState = DMA_IN_PROGRESS;
}

void Gameboy::write8(uint16_t address, uint8_t value) {
	if (dmaCyclesLeft && (address < 0xFF80))
		return;

	switch (address) {
	case 0x0000 ... 0x7FFF: // Cartridge ROM
		rom.write(address, value);
		return;
	case 0x8000 ... 0x9FFF: // VRAM
		ppu.write(address, value);
		return;
	case 0xA000 ... 0xBFFF: // Cartridge RAM
		rom.write(address, value);
		return;
	case 0xC000 ... 0xFDFF: // Work RAM/Echo RAM
		ram.write(address, value);
		return;
	case 0xFE00 ... 0xFE9F: // OAM
		ppu.write(address, value);
		return;
	case 0xFF00: // Joypad
		joypadWrite(value);
		return;
	case 0xFF01 ... 0xFF02: // Serial
		serialWrite(address, value);
		return;
	case 0xFF04 ... 0xFF07: // Timer
		timer.write(address, value);
		return;
	case 0xFF0F:
		cpu.write(address, value);
		return;
	case 0xFF10 ... 0xFF3F: // APU
		apu.write(address, value);
		return;
	case 0xFF40 ... 0xFF4B: // PPU Registers
		ppu.write(address, value);
		return;
	case 0xFF50: // Enable/Disbale Bootrom
		rom.write(address, value);
		return;
	case 0xFF72:
		if (system == SYSTEM_CGB)
			undocumented1 = value;
		return;
	case 0xFF73:
		if (system == SYSTEM_CGB)
			undocumented2 = value;
		return;
	case 0xFF74:
		if (system == SYSTEM_CGB)
			undocumented3 = value;
		return;
	case 0xFF75:
		if (system == SYSTEM_CGB)
			undocumented4 = value & 0x70;
		return;
	case 0xFF80 ... 0xFFFF: // HRAM/Interrupt Enables
		cpu.write(address, value);
		return;
	default:
		return;
	}
}

uint8_t Gameboy::read8(uint16_t address) {
	if (dmaCyclesLeft && (address < 0xFF80))
		return 0xFF;

	switch (address) {
	case 0x0000 ... 0x7FFF: // Cartridge ROM
		return rom.read(address);
	case 0x8000 ... 0x9FFF: // VRAM
		return ppu.read(address);
	case 0xA000 ... 0xBFFF: // Cartridge RAM
		return rom.read(address);
	case 0xC000 ... 0xFDFF: // Work RAM/Echo RAM
		return ram.read(address);
	case 0xFE00 ... 0xFE9F: // OAM
		return ppu.read(address);
	case 0xFF00: // Joypad
		return joypadRead();
	case 0xFF01 ... 0xFF02: // Serial
		return serialRead(address);
	case 0xFF04 ... 0xFF07: // Timer
		return timer.read(address);
	case 0xFF0F:
		return cpu.read(address);
	case 0xFF10 ... 0xFF3F: // APU
		return apu.read(address);
	case 0xFF40 ... 0xFF4B: // PPU Registers
		return ppu.read(address);
	case 0xFF50: // Enable/Disbale Bootrom
		return rom.read(address);
	case 0xFF72:
		if (system == SYSTEM_CGB)
			return undocumented1;
		return 0xFF;
	case 0xFF73:
		if (system == SYSTEM_CGB)
			return undocumented2;
		return 0xFF;
	case 0xFF74:
		if (system == SYSTEM_CGB)
			return undocumented3;
		return 0xFF;
	case 0xFF75:
		if (system == SYSTEM_CGB)
			return undocumented4 | 0x8F;
	case 0xFF80 ... 0xFFFF: // HRAM/Interrupt Enables
		return cpu.read(address);
	default:
		return 0xFF;
	}
}

void Gameboy::write16(uint16_t address, uint16_t value) {
	write8(address, (value & 0xFF));
	write8(address + 1, (value >> 8));

	return;
}

uint16_t Gameboy::read16(uint16_t address) {
	return read8(address) | (read8(address + 1) << 8);
}

void Gameboy::push8(uint8_t value) {
	cpu.r.sp--;
	write8(cpu.r.sp, value);

	return;
}

void Gameboy::push16(uint16_t value) {
	cpu.r.sp -= 2;
	write16(cpu.r.sp, value);

	return;
}

uint8_t Gameboy::pop8() {
	++cpu.r.sp;
	return read8(cpu.r.sp);
}

uint16_t Gameboy::pop16() {
	cpu.r.sp += 2;
	return read16(cpu.r.sp-2);
}
