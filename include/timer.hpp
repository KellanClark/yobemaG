#ifndef GBTIMER_HPP
#define GBTIMER_HPP

class Gameboy;
class GameboyTimer {
public:
	Gameboy& bus;

	GameboyTimer(Gameboy& bus_);
	void cycle(void);
	void write(uint16_t address, uint8_t value);
	uint8_t read(uint16_t address);

	uint16_t divider;	// 0xFF04
	uint8_t counter;	// 0xFF05
	uint8_t modulo;		// 0xFF06
	union {
		struct {
			uint8_t speed : 2;
			uint8_t enable : 1;
			uint8_t test : 5;
		};
		uint8_t value;
	} control;			// 0xFF07

	int andResultPrevious;
	int interruptCountdown;
	bool reloadedThisCycle;
};

#endif
