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
	MBC_7,
	WISDOM_TREE,
	DEFAULT_MBC
};

enum systemType : int;
class Gameboy;
class GameboyCartridge {
public:
	Gameboy& bus;

	GameboyCartridge(Gameboy& gb_);
	~GameboyCartridge();
	void reset();
	void write(uint16_t address, uint8_t value);
	uint8_t read(uint16_t address);
	int load(std::filesystem::path romFilePath_, std::filesystem::path bootromFilePath_, systemType requestedSystem, gbMbcType requestedMbc = DEFAULT_MBC);
	void save();

	// MBC reads and writes
	inline void writeMBC1(uint16_t address, uint8_t value);
	inline uint8_t readMBC1(uint16_t address);
	inline void writeMBC2(uint16_t address, uint8_t value);
	inline uint8_t readMBC2(uint16_t address);
	inline void writeMBC3(uint16_t address, uint8_t value);
	inline uint8_t readMBC3(uint16_t address);
	inline void writeMBC5(uint16_t address, uint8_t value);
	inline uint8_t readMBC5(uint16_t address);
	inline void writeWisdomTree(uint16_t address, uint8_t value);
	inline uint8_t readWisdomTree(uint16_t address);

	// File names and data
	std::filesystem::path romFilePath;
	std::vector<uint8_t> romBuff;
	size_t romSize;
	std::filesystem::path bootromFilePath;
	std::vector<uint8_t> bootromBuff;
	size_t bootromSize;
	std::vector<uint8_t> extRAMBuff;
	bool bootromEnabled;
	std::filesystem::path saveFilePath;

	// MBC variables
	int selectedROMBank;
	int selectedROMBankUpperBits;
	int selectedExtRAMBank;
	bool extRAMEnabled;
	bool advancedBankingMode;

	// Information about the rom
	std::string name;
	enum gbMbcType mbc;
	bool dmgSupported;
	bool cgbSupported;
	bool sgbSupported;
	int extROMBanks;
	int extRAMBanks;
	bool saveBatteryEnabled;
	bool rtcEnabled;
private:
};

#endif
