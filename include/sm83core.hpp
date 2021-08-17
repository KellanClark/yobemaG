#ifndef SM83_HPP
#define SM83_HPP

#include "gameboy.hpp"

class Gameboy;
class SM83 {
public:
	Gameboy& bus;

	SM83(Gameboy& bus_);
	void executeOpcode();

	int mCycle;

	struct {
		uint16_t pc;
		uint16_t sp;

		union {
			struct {
				union {
					struct {
						uint8_t : 4;
						uint8_t C : 1;
						uint8_t H : 1;
						uint8_t N : 1;
						uint8_t Z : 1;
					};
					uint8_t f;
				};
				uint8_t a;		
			};
			uint16_t af;
		};

		union {
			struct {
				uint8_t c;
				uint8_t b;		
			};
			uint16_t bc;
		};

		union {
			struct {
				uint8_t e;
				uint8_t d;		
			};
			uint16_t de;
		};

		union {
			struct {
				uint8_t l;
				uint8_t h;		
			};
			uint16_t hl;
		};
	} r;

	uint8_t opcode;
	uint8_t cbOpcode;
private:
	// Decoding
	uint8_t *decodeReg8(uint8_t operand);
	uint16_t *decodeReg16(uint8_t operand);
	bool checkBranchCondition(uint8_t opcode);
	// Memory
	inline uint8_t fetchByte(uint16_t address);
	inline uint16_t fetchWord(uint16_t address);
	inline void writeByte(uint16_t address, uint8_t value);
	// ALU
	inline void RLC(uint8_t *operand);
	inline void RRC(uint8_t *operand);
	inline void RL(uint8_t *operand);
	inline void RR(uint8_t *operand);
	inline void SLA(uint8_t *operand);
	inline void SRA(uint8_t *operand);
	inline void SWAP(uint8_t *operand);
	inline void SRL(uint8_t *operand);
	inline void BIT(uint8_t operand);
	inline void RES(uint8_t *operand);
	inline void SET(uint8_t *operand);
	inline void ADD(uint8_t operand);
	inline void ADC(uint8_t operand);
	inline void SUB(uint8_t operand);
	inline void SBC(uint8_t operand);
	inline void AND(uint8_t operand);
	inline void XOR(uint8_t operand);
	inline void OR(uint8_t operand);
	inline void CP(uint8_t operand);

	uint8_t *operand8;
	uint16_t *operand16;
	uint8_t memOperand8;
	uint16_t memOperand16;
	uint8_t oldCarry;
};

#endif
