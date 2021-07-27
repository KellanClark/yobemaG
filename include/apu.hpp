#ifndef GBAPU_HPP
#define GBAPU_HPP

class Gameboy;
class GameboyAPU {
public:
	Gameboy& bus;

	GameboyAPU(Gameboy& bus_, void (*sampleBufferFull_)(), int sampleRate_ = 48000);
	void cycle(void);
	void write(uint16_t address, uint8_t value);
	uint8_t read(uint16_t address);
	int calculateSweepFrequency();

	void (*sampleBufferFull)();
	bool tickFrameSequencer;
	int frameSequencerCounter;
	int sampleRate;
	int sampleCounter;
	int sampleBufferIndex;
	std::array<int16_t, 2048> sampleBuffer;

	// Registers
	struct {
		union {
			struct {
				uint8_t sweepShift : 3;
				uint8_t sweepDecrease : 1;
				uint8_t sweepTime : 3;
				uint8_t : 1;
			};
			uint8_t NR10;
		};				// 0xFF10
		union {
			struct {
				uint8_t soundLength : 6;
				uint8_t waveDuty : 2;
			};
			uint8_t NR11;
		};				// 0xFF11
		union {
			struct {
				uint8_t envelopeSweepNum : 3;
				uint8_t envelopeIncrease : 1;
				uint8_t envelopeStartVolume : 4;
			};
			uint8_t NR12;
		};				// 0xFF12
		union {
			uint8_t frequencyLowBits;
			uint8_t NR13;
		};				// 0xFF13
		union {
			struct {
				uint8_t frequencyHighBits : 3;
				uint8_t : 3;
				uint8_t consecutiveSelection : 1;
				uint8_t initial : 1;
			};
			uint8_t NR14;
		};				// 0xFF14
		int frequencyTimer;
		unsigned int waveIndex;
		bool sweepEnabled;
		int shadowFrequency;
		int sweepTimer;
		int lengthCounter;
		int periodTimer;
		int currentVolume;
	} channel1;
	struct {
		union {
			struct {
				uint8_t soundLength : 6;
				uint8_t waveDuty : 2;
			};
			uint8_t NR21;
		};				// 0xFF16
		union {
			struct {
				uint8_t envelopeSweepNum : 3;
				uint8_t envelopeIncrease : 1;
				uint8_t envelopeStartVolume : 4;
			};
			uint8_t NR22;
		};				// 0xFF17
		union {
			uint8_t frequencyLowBits;
			uint8_t NR23;
		};				// 0xFF18
		union {
			struct {
				uint8_t frequencyHighBits : 3;
				uint8_t : 3;
				uint8_t consecutiveSelection : 1;
				uint8_t initial : 1;
			};
			uint8_t NR24;
		};				// 0xFF19
		int frequencyTimer;
		unsigned int waveIndex;
		int lengthCounter;
		int periodTimer;
		int currentVolume;
	} channel2;
	struct {
		union {
			struct {
				uint8_t : 7;
				uint8_t channelOff : 1;
			};
			uint8_t NR30;
		};				// 0xFF1A
		union {
			uint8_t soundLength;
			uint8_t NR31;
		};				// 0xFF1B
		union {
			struct {
				uint8_t : 5;
				uint8_t volume : 2;
				uint8_t : 1;
			};
			uint8_t NR32;
		};				// 0xFF1C
		union {
			uint8_t frequencyLowBits;
			uint8_t NR33;
		};				// 0xFF1D
		union {
			struct {
				uint8_t frequencyHighBits : 3;
				uint8_t : 3;
				uint8_t consecutiveSelection : 1;
				uint8_t initial : 1;
			};
			uint8_t NR34;
		};				// 0xFF1E
		uint8_t waveMem[16]; // 0xFF30 - 0xFF3F
	} channel3;
	struct {
		union {
			struct {
				uint8_t soundLength : 6;
				uint8_t : 2;
			};
			uint8_t NR41;
		};				// 0xFF20
		union {
			struct {
				uint8_t envelopeSweepNum : 3;
				uint8_t envelopeIncrease : 1;
				uint8_t envelopeStartVolume : 4;
			};
			uint8_t NR42;
		};				// 0xFF21
		union {
			struct {
				uint8_t divideRatio : 3;
				uint8_t counterWidth : 1;
				uint8_t shiftClockFrequency : 4;
			};
			uint8_t NR43;
		};				// 0xFF22
		union {
			struct {
				uint8_t : 6;
				uint8_t consecutiveSelection : 1;
				uint8_t initial : 1;
			};
			uint8_t NR44;
		};				// 0xFF23
		int frequencyTimer;
		uint16_t lfsr;
		int lengthCounter;
		int periodTimer;
		int currentVolume;
	} channel4;
	struct {
		union {
			struct {
				uint8_t out1volume : 3;
				uint8_t vinout1 : 1;
				uint8_t out2volume : 3;
				uint8_t vinout2 : 1;
			};
			uint8_t NR50;
		};				// 0xFF24
		union {
			struct {
				uint8_t ch1out1 : 1;
				uint8_t ch2out1 : 1;
				uint8_t ch3out1 : 1;
				uint8_t ch4out1 : 1;
				uint8_t ch1out2 : 1;
				uint8_t ch2out2 : 1;
				uint8_t ch3out2 : 1;
				uint8_t ch4out2 : 1;
			};
			uint8_t NR51;
		};				// 0xFF25
		union {
			struct {
				uint8_t ch1On : 1;
				uint8_t ch2On : 1;
				uint8_t ch3On : 1;
				uint8_t ch4On : 1;
				uint8_t : 3;
				uint8_t allOn : 1;
			};
			uint8_t NR52;
		};				// 0xFF26
		float volumeFloat1;
		float volumeFloat2;
	} soundControl;
};

#endif
