
#include "gameboy.hpp"

/*const int16_t squareWaveDutyCycles[4][8] {
	{-32768, -32768, -32768, -32768, -32768, -32768, -32768,  32767}, // 12.5%
	{ 32767, -32768, -32768, -32768, -32768, -32768, -32768,  32767}, // 25%
	{ 32767, -32768, -32768, -32768, -32768,  32767,  32767,  32767}, // 50%
	{-32768,  32767,  32767,  32767,  32767,  32767,  32767, -32768}  // 75%
};*/

const float squareWaveDutyCycles[4][8] {
	{0, 0, 0, 0, 0, 0, 0, 1}, // 12.5%
	{1, 0, 0, 0, 0, 0, 0, 1}, // 25%
	{1, 0, 0, 0, 0, 1, 1, 1}, // 50%
	{0, 1, 1, 1, 1, 1, 1, 0}  // 75%
};

GameboyAPU::GameboyAPU(Gameboy& bus_, void (*sampleBufferFull_)(), int sampleRate_) : bus(bus_), sampleBufferFull(sampleBufferFull_), sampleRate(sampleRate_) {
	tickFrameSequencer = false;
	frameSequencerCounter = 0;
	sampleCounter = 0;
	sampleBufferIndex = 0;

	return;
}

int GameboyAPU::calculateSweepFrequency() {
	int newFrequency = channel1.shadowFrequency >> channel1.sweepShift;

	if (channel1.sweepDecrease) {
		newFrequency = channel1.shadowFrequency - newFrequency;
	} else {
		newFrequency = channel1.shadowFrequency + newFrequency;
	}

	if (newFrequency > 2047)
		soundControl.ch1On = false;

	return newFrequency;
}

void GameboyAPU::cycle() {
	if (tickFrameSequencer) { // Value calculated in timer
		tickFrameSequencer = false;
		++frameSequencerCounter;

		// Tick Length
		if (!(frameSequencerCounter & 1)) {
			if (channel1.consecutiveSelection) {
				if (channel1.lengthCounter) {
					if ((--channel1.lengthCounter) == 0) {
						soundControl.ch1On = false;
					}
				}
			}
			if (channel2.consecutiveSelection) {
				if (channel2.lengthCounter) {
					if ((--channel2.lengthCounter) == 0) {
						soundControl.ch2On = false;
					}
				}
			}
			if (channel4.consecutiveSelection) {
				if (channel4.lengthCounter) {
					if ((--channel4.lengthCounter) == 0) {
						soundControl.ch2On = false;
					}
				}
			}
		}
		// Tick Volume
		if ((frameSequencerCounter & 7) == 7) {
			if (channel1.periodTimer) {
				if ((--channel1.periodTimer) == 0) {
					channel1.periodTimer = channel1.envelopeSweepNum;
					if ((channel1.currentVolume < 0xF) && channel1.envelopeIncrease) {
						++channel1.currentVolume;
					} else if ((channel1.currentVolume > 0) && !channel1.envelopeIncrease) {
						--channel1.currentVolume;
					}
				}
			}
			if (channel2.periodTimer) {
				if ((--channel2.periodTimer) == 0) {
					channel2.periodTimer = channel2.envelopeSweepNum;
					if ((channel2.currentVolume < 0xF) && channel2.envelopeIncrease) {
						++channel2.currentVolume;
					} else if ((channel2.currentVolume > 0) && !channel2.envelopeIncrease) {
						--channel2.currentVolume;
					}
				}
			}
			if (channel4.periodTimer) {
				//printf("0x%02X\n", channel4.currentVolume);
				if ((--channel4.periodTimer) == 0) {
					channel4.periodTimer = channel4.envelopeSweepNum;
					if ((channel4.currentVolume < 0xF) && channel4.envelopeIncrease) {
						++channel4.currentVolume;
					} else if ((channel4.currentVolume > 0) && !channel4.envelopeIncrease) {
						--channel4.currentVolume;
					}
				}
			}
		}
		// Tick Sweep
		if ((frameSequencerCounter & 3) == 2) {
			if (channel1.sweepTimer) {
				if ((--channel1.sweepTimer) == 0) {
					channel1.sweepTimer = channel1.sweepTime ? channel1.sweepTime : 8;

					if (channel1.sweepEnabled && channel1.sweepTime) {
						int calculatedNewFrequency = calculateSweepFrequency();
						if ((calculatedNewFrequency < 2048) && channel1.sweepShift) {
							channel1.frequencyLowBits = calculatedNewFrequency & 0xFF;
							channel1.frequencyHighBits = (calculatedNewFrequency >> 8) & 7;
							channel1.shadowFrequency = calculatedNewFrequency;

							calculateSweepFrequency();
						}
					}
				}
			}
		}
	}

	if (--channel1.frequencyTimer <= 0) {
		channel1.frequencyTimer = (2048 - ((channel1.frequencyHighBits << 8) | channel1.frequencyLowBits)) * 4;
		channel1.waveIndex = (channel1.waveIndex + 1) & 7;
	}
	if (--channel2.frequencyTimer <= 0) {
		channel2.frequencyTimer = (2048 - ((channel2.frequencyHighBits << 8) | channel2.frequencyLowBits)) * 4;
		channel2.waveIndex = (channel2.waveIndex + 1) & 7;
	}
	if (--channel4.frequencyTimer <= 0) {
		if (channel4.divideRatio) {
			channel4.frequencyTimer = channel4.divideRatio << (channel4.shiftClockFrequency + 4);
		} else {
			channel4.frequencyTimer = 8 << channel4.shiftClockFrequency;
		}
		int xorBit = (channel4.lfsr ^ (channel4.lfsr >> 1)) & 1;
		channel4.lfsr = ((channel4.lfsr >> 1) & 0xBFFF) | (xorBit << 14);
		if (channel4.counterWidth)
			channel4.lfsr = (channel4.lfsr & 0xFFBF) | (xorBit << 6);
		//printf("%d  0x%04X\n", xorBit, channel4.lfsr);
	}

	// Sample audio
	sampleCounter += sampleRate;
	if (sampleCounter >= 4194304) {
		sampleCounter -= 4194304;

		// Sample each channel
		int16_t ch1Sample = soundControl.ch1On * (((channel1.currentVolume * squareWaveDutyCycles[channel1.waveDuty][channel1.waveIndex]) - 7) / 7) * 0x7FFF;
		int16_t ch2Sample = soundControl.ch2On * (((channel2.currentVolume * squareWaveDutyCycles[channel2.waveDuty][channel2.waveIndex]) - 7) / 7) * 0x7FFF;
		int16_t ch3Sample = soundControl.ch3On * 0;
		int16_t ch4Sample = soundControl.ch4On * (((channel4.currentVolume * ((~channel4.lfsr) & 1)) - 7) / 7) * 0x7FFF;

		// Mix and put samples into buffers
		if (soundControl.allOn) {
			sampleBuffer[sampleBufferIndex++] = (((ch1Sample * soundControl.ch1out1) + (ch2Sample * soundControl.ch2out1) + (ch3Sample * soundControl.ch3out1) + (ch4Sample * soundControl.ch4out1)) / 4) * soundControl.volumeFloat1;
			sampleBuffer[sampleBufferIndex++] = (((ch1Sample * soundControl.ch1out2) + (ch2Sample * soundControl.ch2out2) + (ch3Sample * soundControl.ch3out2) + (ch4Sample * soundControl.ch4out2)) / 4) * soundControl.volumeFloat2;
		} else {
			sampleBuffer[sampleBufferIndex++] = 0;
			sampleBuffer[sampleBufferIndex++] = 0;
		}

		// Send samples to device
		if (sampleBufferIndex >= 2048) {
			sampleBufferFull();
			sampleBufferIndex = 0;
		}
	}

	return;
}

void GameboyAPU::write(uint16_t address, uint8_t value) {
	switch (address) {
	case 0xFF10: // NR10
		channel1.NR10 = value & 0x7F;
		return;
	case 0xFF11: // NR11
		channel1.NR11 = value;
		channel1.lengthCounter = 64 - channel1.soundLength;
		return;
	case 0xFF12: // NR12
		channel1.NR12 = value;
		return;
	case 0xFF13: // NR13
		channel1.NR13 = value;
		return;
	case 0xFF14: // NR14
		channel1.NR14 = value & 0xC7;
		if (value & 0x80) {
			channel1.shadowFrequency = (channel1.frequencyHighBits << 8) | channel1.frequencyLowBits;
			channel1.sweepTimer = channel1.sweepTime ? channel1.sweepTime : 8;
			if (channel1.sweepTime || channel1.sweepShift)
				channel1.sweepEnabled = true;
			if (channel1.sweepShift)
				calculateSweepFrequency();
			if (!channel1.lengthCounter)
				channel1.lengthCounter = 64;
			channel1.periodTimer = channel1.envelopeSweepNum;
			channel1.currentVolume = channel1.envelopeStartVolume;
			soundControl.ch1On = true;
		}
		return;
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
			soundControl.ch2On = true;
		}
		return;
	case 0xFF20: // NR41
		channel4.NR41 = value & 0x3F;
		channel4.lengthCounter = 64 - channel4.soundLength;
		return;
	case 0xFF21: // NR42
		channel4.NR42 = value;
		return;
	case 0xFF22: // NR43
		channel4.NR43 = value;
		return;
	case 0xFF23: // NR44
		channel4.NR44 = value & 0xC0;
		if (value & 0x80) {
			channel4.lfsr = 0xFFFF;
			if (!channel4.lengthCounter)
				channel4.lengthCounter = 64;
			channel4.periodTimer = channel4.envelopeSweepNum;
			channel4.currentVolume = channel4.envelopeStartVolume;
			soundControl.ch4On = true;
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
	case 0xFF10: // NR10
		return channel1.NR10 | 0x80;
	case 0xFF11: // NR11
		return channel1.NR11 | 0x3F;
	case 0xFF12: // NR12
		return channel1.NR12;
	case 0xFF13: // NR13
		return 0xFF;
	case 0xFF14: // NR14
		return channel1.NR14 | 0xBF;
	case 0xFF16: // NR21
		return channel2.NR21 | 0x3F;
	case 0xFF17: // NR22
		return channel2.NR22;
	case 0xFF18: // NR23
		return 0xFF;
	case 0xFF19: // NR24
		return channel2.NR24 | 0xBF;
	case 0xFF20: // NR41
		return channel4.NR41 | 0xC0;
	case 0xFF21: // NR42
		return channel4.NR42;
	case 0xFF22: // NR43
		return channel4.NR43;
	case 0xFF23: // NR44
		return channel4.NR44 | 0xBF;
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