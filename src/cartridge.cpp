
#include "../include/gameboy.hpp"

GameboyCartridge::GameboyCartridge(Gameboy& bus_) : bus(bus_) {
	bootromEnabled = false;
	selectedROMBank = 1;
	selectedROMBankUpperBits = 0;

	return;
}

GameboyCartridge::~GameboyCartridge() {
	save();
}

void GameboyCartridge::write(uint16_t address, uint8_t value) {
	// Turn off bootrom on write to 0xFF50
	if ((address == 0xFF50) && bootromEnabled) {
		bootromEnabled = false;
		return;
	}

	// Writes to cartridge
	if ((address <= 0x7FFF) || ((address >= 0xA000) && (address <= 0xBFFF))) {
		switch (mbc) {
			case MBC_1:writeMBC1(address, value);break;
			case MBC_2:writeMBC2(address, value);break;
			case MBC_3:writeMBC3(address, value);break;
			case MBC_5:writeMBC5(address, value);break;
			default:break;
		}
		return;
	}

	return;
}

uint8_t GameboyCartridge::read(uint16_t address) {
	// Read bootrom status
	if (address == 0xFF50)
		return (bootromEnabled | 0xFE); // Bit 0 is the status; the rest are 1s

	// Read from bootrom
	if (bootromEnabled && (address <= 0xFF))
		return bootromBuff[address];
	
	// Read from cartridge ROM
	if (address <= 0x7FFF) {
		switch (mbc) {
			case MBC_1:return readMBC1(address);break;
			case MBC_2:return readMBC2(address);break;
			case MBC_3:return readMBC3(address);break;
			case MBC_5:return readMBC5(address);break;
			default:return romBuff[address];break;
		}
	}

	// Read from cartridge RAM
	if ((address >= 0xA000) && (address <= 0xBFFF)) {
		switch (mbc) {
			case MBC_1:return readMBC1(address);break;
			case MBC_2:return readMBC2(address);break;
			case MBC_3:return readMBC3(address);break;
			case MBC_5:return readMBC5(address);break;
			default:return 0xFF;break;
		}
	}

	return 0xFF; // Default return if memory doesn't exist
}

int GameboyCartridge::load(std::filesystem::path romFilePath_, std::filesystem::path bootromFilePath_, systemType requestedSystem) {
	// Read rom data into memory
	romFilePath = romFilePath_;
	std::ifstream romFileStream;
	romFileStream.open(romFilePath, std::ios::binary);
	if (!romFileStream.is_open()) {
		printf("Failed to open ROM!\n");
		return -1;
	}
	
	romFileStream.seekg(0, std::ios::end);
	romSize = romFileStream.tellg();
	romFileStream.seekg(0, std::ios::beg);
	
	romBuff.resize(romSize);
	romFileStream.read(reinterpret_cast<char*>(romBuff.data()), romBuff.size());
	romFileStream.close();

	if (bootromFilePath_.empty()) { // If no bootrom given, load values and continue
		bootromEnabled = false;
		bus.cpu.r.pc = 0x0100;
		bus.cpu.r.af = 0x01B0;
		bus.cpu.r.bc = 0x0013;
		bus.cpu.r.de = 0x00D8;
		bus.cpu.r.hl = 0x014D;
		bus.cpu.r.sp = 0xFFFE;
	} else { // Read bootrom data into memory
		bootromEnabled = true;
		bus.cpu.r.pc = 0x0000;

		bootromFilePath = bootromFilePath_;
		std::ifstream bootromFileStream;
		bootromFileStream.open(bootromFilePath, std::ios::binary);
		if (!bootromFileStream.is_open()) {
			printf("Failed to open bootrom!");
			return -1;
		}
		
		bootromFileStream.seekg(0, std::ios::end);
		bootromSize = bootromFileStream.tellg();
		bootromFileStream.seekg(0, std::ios::beg);
		
		bootromBuff.resize(bootromSize);
		bootromFileStream.read(reinterpret_cast<char*>(bootromBuff.data()), bootromBuff.size());
		bootromFileStream.close();
	}

	/* Fill game info */
	// Name TODO: fix this
	name.resize(16);
	for (int i = 0; i < 15; i++) {
		name[i] = romBuff[0x134 + i];
	}
	name[15] = romBuff[0x143] & 0x7F;

	// Detect and select system
	if (romBuff[0x143] == 0x80) {
		dmgSupported = true;
		cgbSupported = true;
	} else if (romBuff[0x143] == 0xC0) {
		dmgSupported = false;
		cgbSupported = true;
	} else {
		dmgSupported = true;
		cgbSupported = false;
	}
	if (requestedSystem == SYSTEM_DEFAULT) {
		bus.system = cgbSupported ? SYSTEM_CGB : SYSTEM_DMG;
	} else {
		bus.system = requestedSystem;
		if (((requestedSystem == SYSTEM_DMG) && !dmgSupported) || ((requestedSystem == SYSTEM_CGB) && !cgbSupported))
			printf("Warning: Requested system isn't supported by ROM");
	}
	if ((bus.system == SYSTEM_CGB) && !bootromEnabled) {
		bus.cpu.r.af = 0x11B0;
	}

	// MBC info
	saveBatteryEnabled=false;
	rtcEnabled=false;
	switch (romBuff[0x0147]) {
		case 0x00:
			mbc = NO_MBC;
			break;
		case 0x03:
			saveBatteryEnabled = true;
		case 0x02:
		case 0x01:
			mbc = MBC_1;
			break;
		case 0x05:
			mbc = MBC_2;
			break;
		case 0x06:
			mbc = MBC_2;
			saveBatteryEnabled = true;
			break;
		case 0x0B:
			mbc = MMM01;
			break;
		case 0x0C:
			mbc = MMM01;
			break;
		case 0x0D:
			mbc = MMM01;
			saveBatteryEnabled = true;
			break;
		case 0x0F:
			mbc = MBC_3;
			saveBatteryEnabled = true;
			rtcEnabled = true;
			break;
		case 0x10:
			mbc = MBC_3;
			saveBatteryEnabled = true;
			rtcEnabled = true;
			break;
		case 0x11:
			mbc = MBC_3;
			break;
		case 0x12:
			mbc = MBC_3;
			break;
		case 0x13:
			mbc = MBC_3;
			saveBatteryEnabled = true;
			break;
		case 0x19:
			mbc = MBC_5;
			break;
		case 0x1A:
			mbc = MBC_5;
			break;
		case 0x1B:
			mbc = MBC_5;
			break;
		// TODO: Codes 1C 1D 1E
		case 0x20:
			mbc = MBC_6;
			break;
		// TODO: Codes 22 FC FD FE FF
		default:
			mbc = NO_MBC;
			printf("Unknown MBC Value: 0x%02X\n", romBuff[0x0147]);
			printf("No MBC is assumed\n");
			break;
	}
	if (!mbc) {
		sprintf(mbcString, "None");
	} else if (mbc == 4) {
		sprintf(mbcString, "MMM01");
	} else {
		sprintf(mbcString, "MBC%d", mbc); // TODO: Replace with switch
	}

	if (romBuff[0x0148] <= 8) {
		extROMBanks = 2 << romBuff[0x0148];
	} else {
		extROMBanks = 2;
		printf("Unknown ROM Size: 0x%02X\n2 banks are assumed", romBuff[0x0148]);
	}

	// Detect if game supports SGB functions
	sgbSupported = (romBuff[0x0146] == 3) ? true : false;	// External RAM size
	switch (romBuff[0x0149]) {
	case 0:
		extRAMBanks = 0;
		break;
	case 1:
		extRAMBanks = 0;
		break;
	case 2:
		extRAMBanks = 1;
		break;
	case 3:
		extRAMBanks = 4;
		break;
	case 4:
		extRAMBanks = 16;
		break;
	case 5:
		extRAMBanks = 8;
		break;
	default:
		extRAMBanks = 0;
		printf("Unknown external RAM specifier: 0x%X\n", romBuff[0x0149]);
		printf("No external ram is assumed\n");
		break;
	}
	if (mbc == MBC_2) {
		extRAMBuff.resize(512);
	} else {
		extRAMBuff.resize(extRAMBanks * 8192);
	}

	// Load save file
	if (saveBatteryEnabled) {
		saveFilePath = romFilePath;
		saveFilePath.replace_extension(".sav");
		
		std::ifstream saveFileStream;
		saveFileStream.open(saveFilePath, std::ios::binary);
		saveFileStream.seekg(0, std::ios::beg);
		saveFileStream.read(reinterpret_cast<char*>(extRAMBuff.data()), extRAMBuff.size());
		saveFileStream.close();
	}

	return 0;
}

void GameboyCartridge::writeMBC1(uint16_t address, uint8_t value) {
	if (address <= 0x1FFF) // RAM enable
		extRAMEnabled = ((value & 0x0F) == 0xA) ? true : false;
	
	if ((address >= 0x2000) && (address <= 0x3FFF)) { // Select ROM bank lower bits
		if ((value & 0x1F) == 0) {
			selectedROMBank = (advancedBankingMode ? (selectedROMBank & 0x60) : 0) | 1;
		} else {
			selectedROMBank = (advancedBankingMode ? (selectedROMBank & 0x60) : 0) | (value & 0x1F);
		}
	}

	if ((address >= 0x4000) && (address <= 0x5FFF)) { // Select RAM bank or ROM bank upper bits
		selectedROMBankUpperBits = value & 3;
	}
	
	if ((address >= 0x6000) && (address <= 0x7FFF)) { // Select advanced banking mode
		if (value & 0x1) {
			advancedBankingMode = true;
		} else {
			advancedBankingMode = false;
		}
	}

	if ((address >= 0xA000) && (address <= 0xBFFF) && extRAMEnabled && extRAMBanks) {
		extRAMBuff[(address - 0xA000) + (((advancedBankingMode ? selectedROMBankUpperBits : 0) % extRAMBanks) * 8192)] = value;
	}
}

uint8_t GameboyCartridge::readMBC1(uint16_t address) {
	if (address <= 0x3FFF) {
		return romBuff[address + ((((advancedBankingMode ? selectedROMBankUpperBits : 0) * 32) % extROMBanks) * 16 * 1024)];
	}
	
	if ((address >= 0x4000) && (address <= 0x7FFF)) {
		return romBuff[address + (((((selectedROMBankUpperBits * 32) + selectedROMBank) % extROMBanks) - 1) * 16 * 1024)];
	}

	if ((address >= 0xA000) && (address <= 0xBFFF) && extRAMEnabled && extRAMBanks) {
		return extRAMBuff[(address - 0xA000) + (((advancedBankingMode ? selectedROMBankUpperBits : 0) % extRAMBanks) * 8192)];
	}
	
	return 0xFF;
}

void GameboyCartridge::writeMBC2(uint16_t address, uint8_t value) {
	switch (address) {
	case 0x0000 ... 0x3FFF: // RAM enable/select ROM bank
		if (address & 0x100) { // Select ROM bank
			selectedROMBank = (value % extROMBanks);
			if (!(value & 0xF))
				selectedROMBank = 1;
		} else { // RAM enable
			extRAMEnabled = ((value & 0x0F) == 0xA) ? true : false;
		}
		break;
	case 0xA000 ... 0xBFFF: // External RAM
		if (extRAMEnabled)
			extRAMBuff[(address - 0xA000) & 0x1FF] = value & 0xF;
		break;
	}
}

uint8_t GameboyCartridge::readMBC2(uint16_t address) {
	switch (address) {
	case 0x0000 ... 0x3FFF:
		return romBuff[address];
	case 0x4000 ... 0x7FFF:
		return romBuff[address + ((selectedROMBank - 1) << 14)];
	case 0xA000 ... 0xBFFF:
		return extRAMEnabled ? ((extRAMBuff[(address - 0xA000) & 0x1FF] & 0xF) | 0xF0) : 0xFF;
	}
	
	return 0xFF;
}

void GameboyCartridge::writeMBC3(uint16_t address, uint8_t value) {
	switch (address) {
	case 0x0000 ... 0x1FFF: // RAM and timer enable
		extRAMEnabled = ((value & 0x0F) == 0xA) ? true : false;
		return;
	case 0x2000 ... 0x3FFF:  // Select ROM bank
		selectedROMBank = value ? value % extROMBanks : 1;
		return;
	case 0x4000 ... 0x5FFF: // Select RAM bank or RTC segister
		selectedExtRAMBank = (value & 3) % extRAMBanks;
		return;
	case 0x6000 ... 0x7FFF: // Latch clock data
		// TODO
		return;
	case 0xA000 ... 0xBFFF:
		if (extRAMEnabled && extRAMBanks)
			extRAMBuff[(address - 0xA000) + (selectedExtRAMBank << 13)] = value;
		return;
	}
}

uint8_t GameboyCartridge::readMBC3(uint16_t address) {
	switch (address) {
	case 0x0000 ... 0x3FFF:
		return romBuff[address];
	case 0x4000 ... 0x7FFF:
		return romBuff[address + (((selectedROMBank % extROMBanks) - 1) << 14)];
	case 0xA000 ... 0xBFFF:
		return (extRAMEnabled && extRAMBanks) ? extRAMBuff[(address - 0xA000) + (selectedExtRAMBank << 13)] : 0xFF;
	default:
		return 0xFF;
	}
}

void GameboyCartridge::writeMBC5(uint16_t address, uint8_t value) {
	switch (address) {
	case 0x0000 ... 0x1FFF: // RAM enable
		extRAMEnabled = ((value & 0x0F) == 0xA) ? true : false;
		return;
	case 0x2000 ... 0x2FFF: // Select ROM bank lower bits
		selectedROMBank = (selectedROMBank & 0x100) | value;
		selectedROMBank = (selectedROMBank % extROMBanks);
		return;
	case 0x3000 ... 0x3FFF: // Select ROM bank upper bit
		selectedROMBank = (selectedROMBank & 0xFF) | (value << 8);
		selectedROMBank = (selectedROMBank % extROMBanks);
		return;
	case 0x4000 ... 0x5FFF: // Select RAM bank
		selectedExtRAMBank = (value & 0xF) % extRAMBanks;
		// TODO: Rumble support
		return;
	case 0xA000 ... 0xBFFF: // External RAM
		if (extRAMEnabled && extRAMBanks)
			extRAMBuff[(address - 0xA000) + (selectedExtRAMBank << 13)] = value;
		return;
	}
}

uint8_t GameboyCartridge::readMBC5(uint16_t address) {
	switch (address) {
	case 0x0000 ... 0x3FFF:
		return romBuff[address];
	case 0x4000 ... 0x7FFF:
		return romBuff[address + ((selectedROMBank - 1) << 14)];
	case 0xA000 ... 0xBFFF:
		return (extRAMEnabled && extRAMBanks) ? extRAMBuff[(address - 0xA000) + (selectedExtRAMBank << 13)] : 0xFF;
	}
	
	return 0xFF;
}

void GameboyCartridge::save() {
	if (!saveBatteryEnabled)
		return;
	
	printf("Saving...\n");
	std::ofstream saveFileStream{saveFilePath, std::ios::binary | std::ios::trunc};
	if (!saveFileStream) {
		printf("Trouble Opening Save File!");
		return;
	}
	saveFileStream.write(reinterpret_cast<const char*>(extRAMBuff.data()), extRAMBuff.size());
	saveFileStream.close();
	printf("Save Done!\n");

	return;
}