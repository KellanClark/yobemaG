
#include "gameboy.hpp"

const int16_t squareWaveDutyCycles[4][8] {
	{-32768, -32768, -32768, -32768, -32768, -32768, -32768,  32767}, // 12.5%
	{ 32767, -32768, -32768, -32768, -32768, -32768, -32768,  32767}, // 25%
	{ 32767, -32768, -32768, -32768, -32768,  32767,  32767,  32767}, // 50%
	{-32768,  32767,  32767,  32767,  32767,  32767,  32767, -32768}  // 75%
};

GameboyAPU::GameboyAPU(Gameboy& bus_) : bus(bus_) {
	tickFrameSequencer = false;
	frameSequencerCounter = 0;
	cyclesPerSample = 88;
	sampleCounter = 0;
	sampleIndex = 0;

	return;
}

//int tmp = 0;
void GameboyAPU::cycle() {
	//++tmp;
	if (tickFrameSequencer) { // Value calculated in timer
		tickFrameSequencer = false;
		++frameSequencerCounter;
		//printf("%d\n", tmp);
		//tmp = 0;

		// Tick everything at the right time
		bool tickLength = !(frameSequencerCounter & 1);
		bool tickVolume = ((frameSequencerCounter & 7) == 7);
		bool tickSweep = ((frameSequencerCounter & 3) == 2);

		if (tickLength) {
			if (channel2.consecutiveSelection) {
				if (channel2.lengthCounter) {
					if ((--channel2.lengthCounter) == 0) {
						soundControl.ch2On = false;
					}
				}
			}
		}
		if (tickVolume) {
			if (channel2.periodTimer) {
				if ((--channel2.periodTimer) == 0) {
					channel2.periodTimer = channel2.envelopeSweepNum;
					if ((channel2.currentVolume < 0xF) && channel2.envelopeIncrease) {
						++channel2.currentVolume;
					} else if ((channel2.currentVolume > 0) && !channel2.envelopeIncrease) {
						--channel2.currentVolume;
					}
					channel2.volumeFloat = (float)channel2.currentVolume / 15;
				}
			}
		}
	}

	if (--channel2.frequencyTimer <= 0) {
		channel2.frequencyTimer = (2048 - ((channel2.frequencyHighBits << 8) | channel2.frequencyLowBits)) * 4;
		channel2.waveIndex = (channel2.waveIndex + 1) & 7;
	}

	// Sample audio
	if (++sampleCounter == cyclesPerSample) {
		sampleCounter = 0;

		// Sample each channel
		int16_t ch2Sample = soundControl.ch2On * channel2.volumeFloat * squareWaveDutyCycles[channel2.waveDuty][channel2.waveIndex];

		// Put samples into buffers
		if (soundControl.allOn) {
			sampleBuffer[sampleIndex] = (ch2Sample * soundControl.ch2out1) * soundControl.volumeFloat1;
			sampleBuffer[sampleIndex + 1] = (ch2Sample * soundControl.ch2out2) * soundControl.volumeFloat2;
		} else {
			sampleBuffer[sampleIndex] = 0;
			sampleBuffer[sampleIndex + 1] = 0;
		}

		// Send samples to device
		sampleIndex += 2;
		if (sampleIndex >= 2048) {
			sampleBufferFull();
			sampleIndex = 0;
		}
	}

	return;
}

void GameboyAPU::write(uint16_t address, uint8_t value) {
	switch (address) {
	case 0xFF16: // NR21
		channel2.NR21 = value;
		channel2.lengthCounter = 64 - channel2.soundLength;
		return;
	case 0xFF17: // NR22
		channel2.NR22 = value;
		return;
	case 0xFF18: // NR23
		channel2.NR23 = value;
		return;
	case 0xFF19: // NR24
		channel2.NR24 = value & 0xC7;
		if (value & 0x80) {
			if (!channel2.lengthCounter)
				channel2.lengthCounter = 64;
			channel2.periodTimer = channel2.envelopeSweepNum;
			channel2.currentVolume = channel2.envelopeStartVolume;
			channel2.volumeFloat = (float)channel2.currentVolume / 15;
			soundControl.ch2On = true;
		}
		return;
	case 0xFF24: // NR50
		soundControl.NR50 = value;
		soundControl.volumeFloat1 = (float)soundControl.out1volume / 7;
		soundControl.volumeFloat2 = (float)soundControl.out2volume / 7;
		return;
	case 0xFF25: // NR51
		soundControl.NR51 = value;
		return;
	case 0xFF26: // NR52
		soundControl.NR52 = value & 0x80;
		return;
	default:
		return;
	}
}

uint8_t GameboyAPU::read(uint16_t address) {
	switch (address) {
	case 0xFF16: // NR21
		return channel2.NR21 | 0x3F;
	case 0xFF17: // NR22
		return channel2.NR22;
	case 0xFF18: // NR23
		return 0xFF;
	case 0xFF19: // NR24
		return channel2.NR24 | 0xBF;
	case 0xFF24: // NR50
		return soundControl.NR50;
	case 0xFF25: // NR51
		return soundControl.NR51;
	case 0xFF26: // NR52
		return soundControl.NR52 | 0x70;
	default:
		return 0xFF;
	}
}