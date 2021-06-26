#ifndef GBCART_HPP
#define GBCART_HPP

enum gbMbcType {
	NO_MBC,
	MBC_1,
	MBC_2,
	MBC_3,
	MMM01,	
	MBC_5,
	MBC_6,
	MBC_7
};

class Gameboy;
class GameboyCartridge {
public:
	Gameboy& bus;

	GameboyCartridge(Gameboy& gb_);
	~GameboyCartridge();
	void write(uint16_t address, uint8_t value);
	uint8_t read(uint16_t address);
	int load(std::filesystem::path romFilePath_, std::filesystem::path bootromFilePath_);
	void save();

	// MBC reads and writes
	void writeMBC1(uint16_t address, uint8_t value);
	uint8_t readMBC1(uint16_t address);
	void writeMBC3(uint16_t address, uint8_t value);
	uint8_t readMBC3(uint16_t address);

	// General misc. variables
	std::filesystem::path romFilePath;
	std::vector<uint8_t> romBuff;
	size_t romSize;
	std::filesystem::path bootromFilePath;
	std::vector<uint8_t> bootromBuff;
	size_t bootromSize;
	std::vector<uint8_t> extRAMBuff;
	bool bootromEnabled;
	std::filesystem::path saveFilePath;

	// General misc. variables
	int selectedROMBank;
	int selectedROMBankUpperBits;
	int selectedExtRAMBank;
	bool extRAMEnabled;
	bool advancedBankingMode;

	// Information about the rom
	std::string name;
	enum gbMbcType mbc;
	char mbcString[6];
	bool sgbSupported;
	int extROMBanks;
	int extRAMBanks;
	bool saveBatteryEnabled;
	bool rtcEnabled;
private:
};

#endif
