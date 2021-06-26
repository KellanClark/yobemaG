
#include "gameboy.hpp"

#define BLOCKING_ENABLED 0

GameboyPPU::GameboyPPU(Gameboy& bus_) : bus(bus_) {
	// Clear the VRAM, OAM, and output buffer
	memset(vram, 0, sizeof(vram));
	memset(oam, 0, sizeof(oam));
	memset(outputFramebuffer, 0, sizeof(outputFramebuffer));

	// Just some general starting values
	line=scrollX=scrollY=lineCompare=windowTriggeredThisFrame = 0;
	windowLineCounter = false;
	setMode(PPU_OAM_TRANSFER);
	++modeCycle;

	return;
}

void GameboyPPU::cycle() {
	// Do DMA if requested
	if (bus.dmaCountdown == 1) {
		for (int i = 0; i < 160; i++) {
			oam[i] = bus.read8((dmaAddress << 8) + i);
		}
		bus.dmaCountdown = 0;
		bus.dmaCyclesLeft = 160;
	}
	
	// Don't do anything if turned off
	if (!lcdc.lcdEnable)
		return;

	// Have each mode do its thing
	switch (mode) {
	case PPU_OAM_TRANSFER: // OAM Search
		if (modeCycle == 79)
			setMode(PPU_LCD_TRANSFER);
		
		if (!(modeCycle % 2) &&
			(oamEntries[modeCycle / 2].xPos > 0) &&
			((line + 16) >= oamEntries[modeCycle / 2].yPos) &&
			((line + 16) < (oamEntries[modeCycle / 2].yPos + ((lcdc.objSize + 1) * 8))) &&
			(oamBufferSize < 10)) {
				memcpy(&oamBuffer[oamBufferSize], &oamEntries[modeCycle / 2], sizeof(OamEntry));
				++oamBufferSize;
			}
		break;
	case PPU_LCD_TRANSFER: // LCD Transfer
		if (modeCycle > 11) {
			bgPalettes[modeCycle - 12] = backgroundPalette;
			lcdcValues[modeCycle - 12].value = lcdc.value;
		}

		// Another one of the unholy abominations I dare to call code. View at your own risk
		if (modeCycle == 171) {
			bool drawingWindow;
			uint8_t readData0;
			uint8_t readData1;
			uint8_t readTileNum;
			int pixelNo;
			for (int i = 0; i < 160; i++) {			
				uint8_t *outputBuffPixel = &outputFramebuffer[(line * 160) + i]; // Get index of pixel in framebuffer
				
				// Detect if window or background pixel
				if (lcdcValues[i].windowEnable && windowTriggeredThisFrame && (i >= (windowX - 7))) {
					drawingWindow = true;
				} else {
					drawingWindow = false;
				}
				
				if (drawingWindow) { // Window
					readTileNum = vram[
						(lcdcValues[i].windowTileMapOffset ? 0x1C00 : 0x1800) +	// Get start of tile map
						(((i - (windowX - 7)) / 8) & 0x1f) +						// Offset X
						(32 * (windowLineCounter / 8))];				// Offset Y

					if (lcdcValues[i].tileDataNotOffset) {
						readData0 = vram[(2 * (windowLineCounter % 8)) + (readTileNum * 16)];
						readData1 = vram[(2 * (windowLineCounter % 8)) + (readTileNum * 16) + 1];
					} else {
						readData0 = vram[0x1000 + ((2 * (windowLineCounter % 8)) + ((int8_t)readTileNum * 16))];
						readData1 = vram[0x1000 + ((2 * (windowLineCounter % 8)) + ((int8_t)readTileNum * 16)) + 1];
					}

					pixelNo = 7 - ((i - (windowX - 7)) % 8);
					*outputBuffPixel = ((readData0 & (1 << pixelNo)) >> pixelNo) | (((readData1 & (1 << pixelNo)) >> pixelNo) << 1);
					*outputBuffPixel = (bgPalettes[i] >> (*outputBuffPixel * 2)) & 0x3; // Adjust based on palette
				} else { // Background
					readTileNum = vram[
						(lcdcValues[i].bgTileMapOffset ? 0x1C00 : 0x1800) +	// Get start of tile map
						(((i + scrollX) / 8) & 0x1f) +						// Offset X
						(32 * (((line + scrollY) & 0xFF) / 8))];			// Offset Y
					
					if (lcdcValues[i].tileDataNotOffset) {
						readData0 = vram[(2 * ((line + scrollY) % 8)) + (readTileNum * 16)];
						readData1 = vram[(2 * ((line + scrollY) % 8)) + (readTileNum * 16) + 1];
					} else {
						readData0 = vram[0x1000 + ((2 * ((line + scrollY) % 8)) + ((int8_t)readTileNum * 16))];
						readData1 = vram[0x1000 + ((2 * ((line + scrollY) % 8)) + ((int8_t)readTileNum * 16)) + 1];
					}

					pixelNo = 7 - ((i + scrollX) % 8);
					*outputBuffPixel = ((readData0 & (1 << pixelNo)) >> pixelNo) | (((readData1 & (1 << pixelNo)) >> pixelNo) << 1);
					*outputBuffPixel = (bgPalettes[i] >> (*outputBuffPixel * 2)) & 0x3; // Adjust based on palette
				}

				if (!lcdcValues[i].bgWindowEnable) {
					*outputBuffPixel = backgroundPalette & 3;
				}
			}
			/*
			if (!bus.cgbMode) {
				// Insertion sort by X coordinate "adapted" from tutorialspoint
				OamEntry key;
				int j;
				for (int i = 0; i < oamBufferSize; i++) {
					memcpy(&key, &oamBuffer[i], sizeof(OamEntry));
					j = i;
					while (j > 0 && oamBuffer[j-1].xPos < key.xPos) {
						memcpy(&oamBuffer[j], &oamBuffer[j-1], sizeof(OamEntry));
						j--;
					}
					memcpy(&oamBuffer[j], &key, sizeof(OamEntry));
				}
			}*/
			for (int i = 0; i < oamBufferSize; i++) {
				if (oamBuffer[i].xPos >= 168)
					continue;

				readTileNum = oamBuffer[i].tileIndex;
				if (lcdc.objSize) { // Special things for 8x16 tiles
					readTileNum = oamBuffer[i].tileIndex & ~1; // Ignore first bit
					if (((line - (oamBuffer[i].yPos - 16)) % 16) > 7)
						readTileNum += 1;
					if (oamBuffer[i].yFlip) // Swap fetched tile when y flipped
						readTileNum ^= 1;
				}

				if (oamBuffer[i].yFlip) {
					readData0 = vram[(2 * (7 - ((line - (oamBuffer[i].yPos - 16)) % 8))) + (readTileNum * 16)];
					readData1 = vram[(2 * (7 - ((line - (oamBuffer[i].yPos - 16)) % 8))) + (readTileNum * 16) + 1];
				} else {
					readData0 = vram[(2 * ((line - (oamBuffer[i].yPos - 16)) % 8)) + (readTileNum * 16)];
					readData1 = vram[(2 * ((line - (oamBuffer[i].yPos - 16)) % 8)) + (readTileNum * 16) + 1];
				}

				for (int j = 0; j < 8; j++) {
					uint8_t pixelColor;
					int pixelX = ((oamBuffer[i].xPos - 8) + j);

					// Make sure pixel is on screen
					if ((pixelX >= 160) || (pixelX < 0) || !lcdcValues[pixelX].objEnable)
						continue;

					pixelNo = oamBuffer[i].xFlip ? j : (7 - j);
					pixelColor = ((readData0 & (1 << pixelNo)) >> pixelNo) | (((readData1 & (1 << pixelNo)) >> pixelNo) << 1);
					// Adjust based on palette
					bool pixelTransparent = (pixelColor == 0) ? true : false;
					if (oamBuffer[i].palette) {
						pixelColor = (objectPalette1 >> (pixelColor * 2)) & 0x3;
					} else {
						pixelColor = (objectPalette0 >> (pixelColor * 2)) & 0x3;
					}

					if (oamBuffer[i].priority) {
						if (pixelTransparent)
							pixelColor = 0;
						if (outputFramebuffer[(line * 160) + pixelX] == 0)
							outputFramebuffer[(line * 160) + pixelX] = pixelColor;
					} else {
						if (!pixelTransparent)
							outputFramebuffer[(line * 160) + pixelX] = pixelColor;
					}
				}
			}
			setMode(PPU_HBLANK);
		}
		break;
	case PPU_HBLANK: // HBlank
		if (lineCycle >= 456)
			setMode(PPU_OAM_TRANSFER);
		break;
	case PPU_VBLANK: // VBlank
		// Update line counter
		if (!((modeCycle + 1) % 456) && (line <= 153)) {
			++line;
			checkLCDStatusForInterrupt();
		}
		if (line == 154) {
			line = -1;
			windowLineCounter = 0;
			windowTriggeredThisFrame = false;
			frameDone = true;
			//printf("Frame took %d cycles.\n", frameCycle + 1);
			frameCycle = -1;
			setMode(PPU_OAM_TRANSFER);
		}
		break;
	default:
		break;
	};

	// Increment the cycle counter
	++modeCycle;
	++lineCycle;
	++frameCycle;

	return;
}

void GameboyPPU::write(uint16_t address, uint8_t value) {
	switch (address) {
	case 0x8000 ... 0x9FFF:
		if (vramEnable || !lcdc.lcdEnable || BLOCKING_ENABLED)
			vram[address - 0x8000] = value;
		return;
	case 0xFE00 ... 0xFE9F:
		if (oamEnable || !lcdc.lcdEnable || BLOCKING_ENABLED)
			oam[address - 0xFE00] = value;
		return;
	case 0xFF40: // LCDC
		if (!(value & 0x80) && lcdc.lcdEnable) { // When turning LCD off
			line = -1;
			frameDone = true;
			memset(outputFramebuffer, 0, sizeof(outputFramebuffer));
			setMode(PPU_OAM_TRANSFER);
		}

		lcdc.value = value;
		return;
	case 0xFF41: // STAT
		lcdStatus = (value & 0x78) | (lcdStatus & 7);
		checkLCDStatusForInterrupt();
		return;
	case 0xFF42: // SCY
		scrollY = value;
		return;
	case 0xFF43: // SCX
		scrollX = value;
		return;
	case 0xFF44: // LY
		return;
	case 0xFF45: // LYC
		lineCompare = value;
		checkLCDStatusForInterrupt();
		return;
	case 0xFF46: // DMA
		if (bus.dmaCountdown)
			return;
		bus.dmaCountdown = 2;
		dmaAddress = value;
		if (dmaAddress > 0xDF)
			dmaAddress -= 0x20;
		return;
	case 0xFF47:
		backgroundPalette = value;
		return;
	case 0xFF48:
		objectPalette0 = value;
		return;
	case 0xFF49:
		objectPalette1 = value;
		return;
	case 0xFF4A:
		windowY = value;
		return;
	case 0xFF4B:
		windowX = value;
		return;
	default:
		return;
	}
}

uint8_t GameboyPPU::read(uint16_t address) {
	switch (address) {
	case 0x8000 ... 0x9FFF:
		return (vramEnable || !lcdc.lcdEnable || BLOCKING_ENABLED) ? vram[address - 0x8000] : 0xFF;
	case 0xFE00 ... 0xFE9F:
		return (oamEnable || !lcdc.lcdEnable || BLOCKING_ENABLED) ? oam[address - 0xFE00] : 0xFF;
	case 0xFF40: // LCDC
		return lcdc.value;
	case 0xFF41: // STAT
		return 0x80 | (lcdc.lcdEnable ? lcdStatus : (lcdStatus & 0x78));
	case 0xFF42: // SCY
		return scrollY;
	case 0xFF43: // SCX
		return scrollX;
	case 0xFF44: // LY
		return line;
	case 0xFF45: // LYC
		return lineCompare;
	case 0xFF46: // DMA
		return 0xFF;
	case 0xFF47:
		return backgroundPalette;
	case 0xFF48:
		return objectPalette0;
	case 0xFF49:
		return objectPalette1;
	case 0xFF4A:
		return windowY;
	case 0xFF4B:
		return windowX;
	default:
		return 0xFF;
	}
}

void GameboyPPU::setMode(enum ppuModes newMode) {
	//printf("Mode %d took %d cycles.\n", mode, modeCycle + 1);
	switch (newMode) {
	case PPU_HBLANK:
		// Using nothing
		oamEnable = true;
		vramEnable = true;
		
		mode = PPU_HBLANK;
		modeCycle = -1;
		break;
	case PPU_VBLANK:
		// Using nothing
		oamEnable = true;
		vramEnable = true;

		// VBlank can also give its own interrupt
		bus.cpu.requestInterrupt(INT_VBLANK);

		mode = PPU_VBLANK;
		modeCycle = -1;
		break;
	case PPU_OAM_TRANSFER:
		// Increment line counter and trigger Vblank if needed
		++line;
		if (line >= 144) {
			setMode(PPU_VBLANK);
			return;
		}

		// Check if window triggered
		if (windowTriggeredThisFrame && lcdc.windowEnable && ((windowX + 7) < 160)) {
			++windowLineCounter;
		}
		if (windowY == line) {
			windowTriggeredThisFrame = true;
		}

		// Reset variables
		oamBufferSize = 0;

		// Using OAM
		oamEnable = false;
		vramEnable = true;

		mode = PPU_OAM_TRANSFER;
		modeCycle = -1;
		lineCycle = 0;
		break;
	case PPU_LCD_TRANSFER:
		// Using VRAM and OAM
		oamEnable = false;
		vramEnable = false;

		mode = PPU_LCD_TRANSFER;
		modeCycle = -1;
		break;
	default:
		break;
	}

	checkLCDStatusForInterrupt();
	return;
}

void GameboyPPU::checkLCDStatusForInterrupt() {
	bool conditionsMet = false;
	if (interruptLYC && (line == lineCompare)) {
		lineCoincidence = true;
		conditionsMet = true;
	} else {
		lineCoincidence = false;
	}
	if (interruptOAM && (mode == PPU_OAM_TRANSFER))
		conditionsMet = true;
	if (interruptVBlank && (mode == PPU_VBLANK))
		conditionsMet = true;
	if (interruptHBlank && (mode == PPU_HBLANK))
		conditionsMet = true;
	
	if (!conditionsMetPrevious && conditionsMet) // STAT blocking
		bus.cpu.requestInterrupt(INT_LCD);
	
	conditionsMetPrevious = conditionsMet;

	return;
}