
#include "gameboy.hpp"

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
	for (int t = 0; t < 4; t++) {
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
				if (channel3.consecutiveSelection) {
					if (channel3.lengthCounter) {
						if ((--channel3.lengthCounter) == 0) {
							soundControl.ch3On = false;
						}
					}
				}
				if (channel4.consecutiveSelection) {
					if (channel4.lengthCounter) {
						if ((--channel4.lengthCounter) == 0) {
							soundControl.ch4On = false;
						}
					}
				}
			}
			// Tick Volume
			if ((frameSequencerCounter & 7) == 7) {
				if (channel1.periodTimer) {
					if ((--channel1.periodTimer) == 0) {
						channel1.periodTimer = channel1.envelopeSweepNum;
						if (channel1.periodTimer == 0) {
							channel1.periodTimer = 8;
						}
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
						if (channel2.periodTimer == 0) {
							channel2.periodTimer = 8;
						}
						if ((channel2.currentVolume < 0xF) && channel2.envelopeIncrease) {
							++channel2.currentVolume;
						} else if ((channel2.currentVolume > 0) && !channel2.envelopeIncrease) {
							--channel2.currentVolume;
						}
					}
				}
				if (channel4.periodTimer) {
					if ((--channel4.periodTimer) == 0) {
						channel4.periodTimer = channel4.envelopeSweepNum;
						if (channel4.periodTimer == 0) {
							channel4.periodTimer = 8;
						}
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
		if (--channel3.frequencyTimer <= 0) {
			channel3.frequencyTimer = (2048 - ((channel3.frequencyHighBits << 8) | channel3.frequencyLowBits)) * 2;
			#ifdef CURSED_WAVE_RAM
			channel3.waveMemIndex = (channel3.waveMemIndex - 1) & 0x1F;
			channel3.waveMemShiftNo = channel3.waveMemIndex * 4;
			#else
			channel3.waveMemIndex = (channel3.waveMemIndex + 1) & 0x1F;
			#endif
		}
		if (--channel4.frequencyTimer <= 0) {
			if (channel4.divideRatio) {
				channel4.frequencyTimer = channel4.divideRatio << (channel4.shiftClockFrequency + 4);
			} else {
				channel4.frequencyTimer = 8 << channel4.shiftClockFrequency;
			}
			int xorBit = (channel4.lfsr ^ (channel4.lfsr >> 1)) & 1;
			channel4.lfsr = (channel4.lfsr >> 1) | (xorBit << 14);
			if (channel4.counterWidth)
				channel4.lfsr = (channel4.lfsr & 0xFFBF) | (xorBit << 6);
		}

		// Sample audio
		sampleCounter += sampleRate;
		if (sampleCounter >= 4194304) {
			sampleCounter -= 4194304;

			// Sample each channel
			float ch1Sample = soundControl.ch1On * channel1.dacOn * (((channel1.currentVolume * squareWaveDutyCycles[channel1.waveDuty][channel1.waveIndex]) / 7.5) - 1.0f);
			float ch2Sample = soundControl.ch2On * channel2.dacOn * (((channel2.currentVolume * squareWaveDutyCycles[channel2.waveDuty][channel2.waveIndex]) / 7.5) - 1.0f);
			#ifdef CURSED_WAVE_RAM
			float ch3Sample = soundControl.ch3On * channel3.dacOn * (((((channel3.waveMemInt & ((__uint128_t)0xF << channel3.waveMemShiftNo)) >> channel3.waveMemShiftNo) >> (channel3.volume ? (channel3.volume - 1) : 4)) / 7.5) - 1.0f);
			#else
			float ch3Sample = soundControl.ch3On * channel3.dacOn * (((channel3.waveMem[channel3.waveMemIndex] >> (channel3.volume ? (channel3.volume - 1) : 4)) / 7.5) - 1.0f);
			#endif
			float ch4Sample = soundControl.ch4On * channel4.dacOn * (((channel4.currentVolume * ((~channel4.lfsr) & 1)) / 7.5) - 1.0f);

			// Mix and put samples into buffers
			if (soundControl.allOn) {
				sampleBuffer[sampleBufferIndex++] = (((ch1Sample * soundControl.ch1out1) + (ch2Sample * soundControl.ch2out1) + (ch3Sample * soundControl.ch3out1) + (ch4Sample * soundControl.ch4out1)) / 4) * soundControl.volumeFloat1 * 0x7FFF;
				sampleBuffer[sampleBufferIndex++] = (((ch1Sample * soundControl.ch1out2) + (ch2Sample * soundControl.ch2out2) + (ch3Sample * soundControl.ch3out2) + (ch4Sample * soundControl.ch4out2)) / 4) * soundControl.volumeFloat2 * 0x7FFF;
			} else {
				sampleBuffer[sampleBufferIndex++] = 0;
				sampleBuffer[sampleBufferIndex++] = 0;
			}

			// Send samples to device
			if (sampleBufferIndex >= 2048) {
				sampleBufferFull();
			}
		}
	}

	return;
}

void GameboyAPU::write(uint16_t address, uint8_t value) {
	if (!soundControl.allOn && (address >= 0xFF10) && (address <= 0xFF25))
		return;

	switch (address) {
	case 0xFF10: // NR10
		channel1.NR10 = value;
		return;
	case 0xFF11: // NR11
		channel1.NR11 = value;
		channel1.lengthCounter = 64 - channel1.soundLength;
		return;
	case 0xFF12: // NR12
		channel1.NR12 = value;
		channel1.dacOn = (value & 0xF8) != 0;
		if (!channel1.dacOn)
			soundControl.ch1On = false;
		return;
	case 0xFF13: // NR13
		channel1.NR13 = value;
		return;
	case 0xFF14: // NR14
		if (value & 0x80) {
			channel1.shadowFrequency = ((value & 7) << 8) | channel1.frequencyLowBits;
			channel1.sweepTimer = channel1.sweepTime ? channel1.sweepTime : 8;
			if (channel1.sweepTime || channel1.sweepShift)
				channel1.sweepEnabled = true;
			if (channel1.sweepShift)
				calculateSweepFrequency();
			if (!channel1.lengthCounter) {
				/*if ((value & 0x40) && // New value enables length
					!channel1.consecutiveSelection && // Length disabled until now
					(frameSequencerCounter & 1)) { // Frame sequencer won't clock length next
					channel1.lengthCounter = 63;
				} else {
					channel1.lengthCounter = 64;
				}*/
				channel1.lengthCounter = 64;
			}
			//if ((value & 0x40) && !(channel1.NR14 & 0x40))
			//	--channel1.lengthCounter;
			channel1.periodTimer = channel1.envelopeSweepNum;
			channel1.currentVolume = channel1.envelopeStartVolume;
			if (channel1.dacOn)
				soundControl.ch1On = true;
		}
		if ((value & 0x40) && !channel1.consecutiveSelection && !(frameSequencerCounter & 1)) {
			if (channel1.lengthCounter) {
				if ((--channel1.lengthCounter) == 0) {
					soundControl.ch1On = false;
				}
			}
		}
		channel1.NR14 = value;
		return;
	case 0xFF16: // NR21
		channel2.NR21 = value;
		channel2.lengthCounter = 64 - channel2.soundLength;
		return;
	case 0xFF17: // NR22
		channel2.NR22 = value;
		channel2.dacOn = (value & 0xF8) != 0;
		if (!channel2.dacOn)
			soundControl.ch2On = false;
		return;
	case 0xFF18: // NR23
		channel2.NR23 = value;
		return;
	case 0xFF19: // NR24
		if (value & 0x80) {
			if (!channel2.lengthCounter)
				channel2.lengthCounter = 64;
			channel2.periodTimer = channel2.envelopeSweepNum;
			channel2.currentVolume = channel2.envelopeStartVolume;
			if (channel2.dacOn)
				soundControl.ch2On = true;
		}
		if ((value & 0x40) && !channel2.consecutiveSelection && !(frameSequencerCounter & 1)) {
			if (channel2.lengthCounter) {
				if ((--channel2.lengthCounter) == 0) {
					soundControl.ch2On = false;
				}
			}
		}
		channel2.NR24 = value;
		return;
	case 0xFF1A: // NR30
		channel3.NR30 = value;
		if (!channel3.dacOn)
			soundControl.ch3On = false;
		return;
	case 0xFF1B: // NR31
		channel3.NR31 = value;
		channel3.lengthCounter = 256 - channel3.soundLength;
		return;
	case 0xFF1C: // NR32
		channel3.NR32 = value;
		return;
	case 0xFF1D: // NR33
		channel3.NR33 = value;
		return;
	case 0xFF1E: // NR34
		if (value & 0x80) {
			if (!channel3.lengthCounter)
				channel3.lengthCounter = 256;
			if (channel3.dacOn)
				soundControl.ch3On = true;
		}
		if ((value & 0x40) && !channel3.consecutiveSelection && !(frameSequencerCounter & 1)) {
			if (channel3.lengthCounter) {
				if ((--channel3.lengthCounter) == 0) {
					soundControl.ch3On = false;
				}
			}
		}
		channel3.NR34 = value;
		return;
	case 0xFF20: // NR41
		channel4.NR41 = value;
		channel4.lengthCounter = 64 - channel4.soundLength;
		return;
	case 0xFF21: // NR42
		channel4.NR42 = value;
		channel4.dacOn = (value & 0xF8) != 0;
		if (!channel4.dacOn)
			soundControl.ch4On = false;
		return;
	case 0xFF22: // NR43
		channel4.NR43 = value;
		return;
	case 0xFF23: // NR44
		if (value & 0x80) {
			channel4.lfsr = 0x7FFF;
			if (!channel4.lengthCounter)
				channel4.lengthCounter = 64;
			channel4.periodTimer = channel4.envelopeSweepNum;
			channel4.currentVolume = channel4.envelopeStartVolume;
			if (channel4.dacOn)
				soundControl.ch4On = true;
		}
		if ((value & 0x40) && !channel4.consecutiveSelection && !(frameSequencerCounter & 1)) {
			if (channel4.lengthCounter) {
				if ((--channel4.lengthCounter) == 0) {
					soundControl.ch4On = false;
				}
			}
		}
		channel4.NR44 = value;
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
		if (!(value & 0x80)) { // Everything gets cleared when the APU is turned off
			channel1.NR10 = 0;
			channel1.NR11 = 0;
			channel1.NR12 = 0;
			channel1.NR13 = 0;
			channel1.NR14 = 0;
			channel2.NR21 = 0;
			channel2.NR22 = 0;
			channel2.NR23 = 0;
			channel2.NR24 = 0;
			channel3.NR30 = 0;
			channel3.NR31 = 0;
			channel3.NR32 = 0;
			channel3.NR33 = 0;
			channel3.NR34 = 0;
			channel4.NR41 = 0;
			channel4.NR42 = 0;
			channel4.NR43 = 0;
			channel4.NR44 = 0;
			soundControl.NR50 = 0;
			soundControl.NR51 = 0;
			soundControl.NR52 = 0;
		}
		soundControl.NR52 |= value & 0x80;
		return;
	case 0xFF30 ... 0xFF3F: // Wave RAM
		#ifdef CURSED_WAVE_RAM
		channel3.waveMemArray[0xF - (address & 0xF)] = value;
		#else
		channel3.waveMem[(address & 0xF) << 1] = value >> 4;
		channel3.waveMem[((address & 0xF) << 1) + 1] = value & 0xF;
		#endif
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
	case 0xFF1A: // NR30
		return channel3.NR30 | 0x7F;
	case 0xFF1B: // NR31
		return 0xFF;
	case 0xFF1C: // NR32
		return channel3.NR32 | 0x9F;
	case 0xFF1D: // NR33
		return 0xFF;
	case 0xFF1E: // NR34
		return channel3.NR34 | 0xBF;
	case 0xFF20: // NR41
		return 0xFF;
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
	case 0xFF30 ... 0xFF3F: // Wave RAM
		#ifdef CURSED_WAVE_RAM
		return channel3.waveMemArray[0xF - (address & 0xF)];
		#else
		return (channel3.waveMem[(address & 0xF) << 1] << 4) | channel3.waveMem[((address & 0xF) << 1) + 1];
		#endif
	default:
		return 0xFF;
	}
}
