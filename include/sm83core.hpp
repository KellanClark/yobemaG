#ifndef SM83_HPP
#define SM83_HPP

#include "gameboy.hpp"

class Gameboy;
class SM83 {
public:
	Gameboy& bus;

	SM83(Gameboy& bus_);
	void executeOpcode();
	void executeOpcodeCA();

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
	uint8_t fetchByte(uint16_t address);
	uint16_t fetchWord(uint16_t address);
	void writeByte(uint16_t address, uint8_t value);
	// ALU
	void RLC(uint8_t *operand);
	void RRC(uint8_t *operand);
	void RL(uint8_t *operand);
	void RR(uint8_t *operand);
	void SLA(uint8_t *operand);
	void SRA(uint8_t *operand);
	void SWAP(uint8_t *operand);
	void SRL(uint8_t *operand);
	void BIT(uint8_t operand);
	void RES(uint8_t *operand);
	void SET(uint8_t *operand);
	void ADD(uint8_t operand);
	void ADC(uint8_t operand);
	void SUB(uint8_t operand);
	void SBC(uint8_t operand);
	void AND(uint8_t operand);
	void XOR(uint8_t operand);
	void OR(uint8_t operand);
	void CP(uint8_t operand);

	uint8_t *operand8;
	uint16_t *operand16;
	uint8_t memOperand8;
	uint16_t memOperand16;
	uint8_t oldCarry;
};

#endif
