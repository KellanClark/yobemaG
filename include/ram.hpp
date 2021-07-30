#ifndef GBRAM_HPP
#define GBRAM_HPP


class Gameboy;
class GameboyRAM {
public:
	Gameboy& bus;

	GameboyRAM(Gameboy& bus_);
	void reset();
	void write(uint16_t address, uint8_t value);
	uint8_t read(uint16_t address);

	uint8_t ram[0x2000];

private:
};

#endif
