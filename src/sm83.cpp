
#include "../include/gameboy.hpp"

// Easy ways to set flags x=arg1 y=arg2 z=carry (0 if not used)
#define setZ(x) (r.Z = x)
#define testZ(x) (r.Z = (x ? 0 : 1))
#define setN(x) (r.N = x)
#define setH(x) (r.H = x)
#define testHAdd(x, y, z) (r.H = ((((x & 0xF) + (y & 0xF) + z) > 0x0F) ? 1 : 0))
#define testHSub(x, y, z) (r.H = (((x & 0xF) < ((y & 0xF) + z)) ? 1 : 0))
#define setC(x) (r.C = x)
#define testCAdd(x, y, z) (r.C = (((x + y + z) > 0xFF) ? 1 : 0))
#define testCSub(x, y, z) (r.C = ((x < (y + z)) ? 1 : 0))

SM83::SM83(Gameboy& bus_) : bus(bus_) {
	r.af=r.bc=r.de=r.hl=r.pc=r.sp = 0; // Clear registers
}

// vvv Let's make some spaghetti! vvv
void SM83::executeOpcode() {
	opcode = fetchByte(r.pc++);

	if (opcode == 0xCB) { // Prefix CB instructions
		cbOpcode = fetchByte(r.pc++);
		operand8 = decodeReg8(cbOpcode);
		switch (cbOpcode & 0xF8) {
		case 0x00: // RLC
			RLC(operand8);
			break;
		case 0x08: // RRC reg8
			RRC(operand8);
			break;
		case 0x10: // RL reg8
			RL(operand8);
			break;
		case 0x18: // RR reg8
			RR(operand8);
			break;
		case 0x20: // SLA reg8
			SLA(operand8);
			break;
		case 0x28: // SRA reg8
			SRA(operand8);
			break;
		case 0x30: // SWAP reg8
			SWAP(operand8);
			break;
		case 0x38: // SRL reg8
			SRL(operand8);
			break;
		case 0x80 ... 0xBF: // RES bit, reg8
			RES(operand8);
			break;
		case 0xC0 ... 0xFF: // SET bit, reg8
			SET(operand8);
			break;
		}
		
		if ((cbOpcode >= 0x40) && (cbOpcode <= 0x7F)) { // BIT bit, reg8
			BIT(*operand8);
			return;
		} else if ((cbOpcode & 0x07) == 6) {
			writeByte(r.hl, memOperand8);
		}
		return;
	}

	// Take care of instructions with only one opcode or that don't have a pattern
	switch (opcode) {
	case 0x00: // NOP
		return;
	case 0x02: // LD (BC), A
		writeByte(r.bc, r.a);
		return;
	case 0x07: // RLCA
		RLC(&r.a);
		setZ(0);
		return;
	case 0x08: // LD (imm16), SP
		bus.cycleSystem();
		bus.cycleSystem();
		bus.cycleSystem();
		bus.cycleSystem();
		bus.write16(bus.read16(r.pc), r.sp);
		r.pc += 2;
		return;
	case 0x0A: // LD A, (BC)
		r.a = fetchByte(r.bc);
		return;
	case 0x0F: // RRCA
		RRC(&r.a);
		setZ(0);
		return;
	case 0x10: // STOP
		bus.cpu.stopped = true;
		++r.pc;
		return;
	case 0x12: // LD (DE), A
		writeByte(r.de, r.a);
		return;
	case 0x17: // RLA
		RL(&r.a);
		setZ(0);
		return;
	case 0x18: // JR imm8
		bus.cycleSystem();
		r.pc += (int8_t)fetchByte(r.pc++);
		return;
	case 0x1A: // LD A, (DE)
		r.a = fetchByte(r.de);
		return;
	case 0x1F: // RRA
		RR(&r.a);
		setZ(0);
		return;
	case 0x22: // LD (HL+), A
		writeByte(r.hl++, r.a);
		return;
	case 0x27: // DAA
		// I gave up trying to understand this instruction so I just copied this code from some forum
		if (!r.N) {
			if (r.C || r.a > 0x99) {
				r.a += 0x60;
				setC(1);
			}
			if (r.H || (r.a & 0x0f) > 0x09)
				r.a += 0x6;
		} else {
			if (r.C)
				r.a -= 0x60;
			if (r.H)
				r.a -= 0x6;
		}
		testZ(r.a);
		setH(0);
		return;
	case 0x2A: // LD A, (HL+)
		r.a = fetchByte(r.hl++);
		return;
	case 0x2F: // CPL
		setN(1);
		setH(1);
		r.a = ~r.a;
		return;
	case 0x32: // LD (HL-), A
		writeByte(r.hl--, r.a);
		return;
	case 0x34: // INC (HL)
		memOperand8 = fetchByte(r.hl);
		setN(0);
		testHAdd(memOperand8, 1, 0);
		testZ(++memOperand8);
		writeByte(r.hl, memOperand8);
		return;
	case 0x35: // DEC (HL)
		memOperand8 = fetchByte(r.hl);
		setN(1);
		testHSub(memOperand8, 1, 0);
		testZ(--memOperand8);
		writeByte(r.hl, memOperand8);
		return;
	case 0x36: // LD (HL), imm8
		writeByte(r.hl, fetchByte(r.pc++));
		return;
	case 0x37: // SCF
		setN(0);
		setH(0);
		setC(1);
		return;
	case 0x3A: // LD A, (HL-)
		r.a = fetchByte(r.hl--);
		return;
	case 0x3F: // CCF
		setN(0);
		setH(0);
		setC(~r.C);
		return;
	case 0x76: // HALT
		bus.cpu.halted = true;
		return;
	case 0xC3: // JP imm16
		memOperand16 = fetchByte(r.pc++);
		memOperand16 |= fetchByte(r.pc++) << 8;
		bus.cycleSystem();
		r.pc = memOperand16;
		/*bus.cycleSystem();
		r.pc = fetchWord(r.pc);*/
		return;
	case 0xC9: // RET
		bus.cycleSystem();
		bus.cycleSystem();
		bus.cycleSystem();
		r.pc = bus.pop16();
		return;
	case 0xCD: // CALL imm16
		memOperand16 = fetchByte(r.pc++);
		memOperand16 |= fetchByte(r.pc++) << 8;
		bus.cycleSystem();
		writeByte(--r.sp, r.pc >> 8);
		writeByte(--r.sp, r.pc & 0xFF);
		r.pc = memOperand16;
		return;
	case 0xD9: // RETI
		bus.cycleSystem();
		bus.cycleSystem();
		bus.cycleSystem();
		r.pc = bus.pop16();
		bus.cpu.interruptMasterEnable = true;
		return;
	case 0xE0: // LD (FF00+imm8), A
		writeByte((0xFF00 | fetchByte(r.pc++)), r.a);
		return;
	case 0xE2: // LD (FF00+C), A
		writeByte((0xFF00 | r.c), r.a);
		return;
	case 0xE8: // ADD SP, imm8
		setZ(0);
		setN(0);
		memOperand8 = fetchByte(r.pc++);
		setC((((r.sp & 0xFF) + ((int8_t)memOperand8 & 0xFF)) > 0xFF) ? 1 : 0);
		testHAdd(r.sp, (int8_t)memOperand8, 0);
		r.sp += (int8_t)memOperand8;
		bus.cycleSystem();
		bus.cycleSystem();
		return;
	case 0xE9: // JP HL
		r.pc = r.hl;
		return;
	case 0xEA: // LD (imm16), A
		writeByte(fetchWord(r.pc), r.a);
		r.pc += 2;
		return;
	case 0xF0: // LD A, (FF00+imm8)
		r.a = fetchByte(0xFF00 + fetchByte(r.pc++));
		return;
	case 0xF1:
		bus.cycleSystem();
		bus.cycleSystem();
		r.af = bus.pop16() | 0xF;
		return;
	case 0xF2: // LD A, (FF00+C)
		r.a = fetchByte(0xFF00 + r.c);
		return;
	case 0xF3: // DI
		bus.cpu.interruptMasterEnable = false;
		return;
	case 0xF8: // LD HL, SP+imm8
		bus.cycleSystem();
		setZ(0);
		setN(0);
		memOperand8 = fetchByte(r.pc++);
		setC((((r.sp & 0xFF) + ((int8_t)memOperand8 & 0xFF)) > 0xFF) ? 1 : 0);
		testHAdd(r.sp, (int8_t)memOperand8, 0);
		r.hl = r.sp + (int8_t)memOperand8;
		return;
	case 0xF9: // LD SP, HL
		bus.cycleSystem();
		r.sp = r.hl;
		return;
	case 0xFA: // LD A, (imm16)
		r.a = fetchByte(fetchWord(r.pc));
		r.pc += 2;
		return;
	case 0xFB: // EI
		bus.cpu.interruptMasterEnable = true;
		return;
	// Illegal opcodes
	case 0xD3:
	case 0xDB:
	case 0xDD:
	case 0xE3:
	case 0xE4:
	case 0xEB:
	case 0xEC:
	case 0xED:
	case 0xF4:
	case 0xFC:
	case 0xFD:
		printf("Opcode 0x%02X not found. Memory: 0x%04X\n", opcode, r.pc - 1);
		bus.cpu.stopped = true;
		return;
	}

	if (opcode <= 0x3F) {
		if (((opcode & 0x07) >= 1) && ((opcode & 0x07) <= 3)) {
			operand16 = decodeReg16(opcode);
			switch (opcode & 0x0F) {
			case 0x1: // LD reg16, u16
				*operand16 = fetchWord(r.pc);
				r.pc += 2;
				return;
			case 0x3: // INC reg16
				bus.cycleSystem();
				++*operand16;
				return;
			case 0x9: // ADD HL, reg16
				bus.cycleSystem();
				setN(0);
				setC(((r.hl + *operand16) > 0xFFFF) ? 1 : 0);
				setH((((r.hl & 0xFFF) + (*operand16 & 0xFFF)) > 0x0FFF) ? 1 : 0);
				r.hl += *operand16;
				return;
			case 0xB: // DEC reg16
				bus.cycleSystem();
				--*operand16;
				return;
			}
		}
		switch (opcode & 0x07) {
		case 0: // JR cond, imm8
			memOperand8 = fetchByte(r.pc++);
			if (checkBranchCondition(opcode)) {
				bus.cycleSystem();
				r.pc += (int8_t)memOperand8;
			}
			return;
		case 4: // INC reg8
			operand8 = decodeReg8(opcode >> 3);
			setN(0);
			testHAdd(*operand8, 1, 0);
			testZ(++*operand8);
			return;
		case 5: // DEC reg8
			operand8 = decodeReg8(opcode >> 3);
			setN(1);
			testHSub(*operand8, 1, 0);
			testZ(--*operand8);
			return;
		case 6: // LD reg8, imm8
			*decodeReg8(opcode >> 3) = fetchByte(r.pc++);
			break;
		}
	}

	if ((opcode >= 0x40) && (opcode <= 0xBF)) { // All take the same operand
		operand8 = decodeReg8(opcode);
		if (opcode <= 0x7F) { // LD instructions
			if ((opcode >= 0x70) && (opcode <= 0x77)) { // LD (HL), reg8
				writeByte(r.hl, *operand8);
				return;
			}
			// LD reg8, reg8
			*decodeReg8(opcode >> 3) = *operand8;
			return;
		}
		switch (opcode & 0xF8) { // 8-bit ALU operations
		case 0x80: // ADD A, reg8
			ADD(*operand8);
			return;
		case 0x88: // ADC A, reg8
			ADC(*operand8);
			return;
		case 0x90: // SUB A, reg8
			SUB(*operand8);
			return;
		case 0x98: // SBC A, reg8
			SBC(*operand8);
			return;
		case 0xA0: // AND A, reg8
			AND(*operand8);
			return;
		case 0xA8: // XOR A, reg8
			XOR(*operand8);
			return;
		case 0xB0: // OR A, reg8
			OR(*operand8);
			return;
		case 0xB8: // CP A, reg8
			CP(*operand8);
			return;
		}
	}
	if (opcode >= 0xC0) {
		switch (opcode & 0x07) {
		case 0: // RET cond
			bus.cycleSystem();
			if (checkBranchCondition(opcode)) {
				bus.cycleSystem();
				bus.cycleSystem();
				bus.cycleSystem();
				r.pc = bus.pop16();
			}
			return;
		case 1: // POP reg16
			bus.cycleSystem();
			bus.cycleSystem();
			*decodeReg16(opcode) = bus.pop16();
			return;
		case 2: // JP cond, imm16
			memOperand16 = fetchWord(r.pc);
			r.pc += 2;
			if (checkBranchCondition(opcode)) {
				bus.cycleSystem();
				r.pc = memOperand16;
			}
			return;
		case 4: // CALL cond, imm16
			memOperand16 = fetchWord(r.pc);
			r.pc += 2;
			if (checkBranchCondition(opcode)) {
				bus.cycleSystem();
				bus.cycleSystem();
				bus.cycleSystem();
				bus.push16(r.pc);
				r.pc = memOperand16;
			}
			return;
		case 5: // PUSH reg16
			bus.cycleSystem();
			bus.cycleSystem();
			bus.cycleSystem();
			bus.push16((opcode == 0xF5) ? r.af : *decodeReg16(opcode));
			return;
		case 6: // ALU A, imm8
			memOperand8 = fetchByte(r.pc++);
			switch (opcode) {
			case 0xC6:ADD(memOperand8);return;
			case 0xCE:ADC(memOperand8);return;
			case 0xD6:SUB(memOperand8);return;
			case 0xDE:SBC(memOperand8);return;
			case 0xE6:AND(memOperand8);return;
			case 0xEE:XOR(memOperand8);return;
			case 0xF6:OR(memOperand8);return;
			case 0xFE:CP(memOperand8);return;
			}
			break;
		case 7: // RST vector
			bus.cycleSystem();
			bus.cycleSystem();
			bus.cycleSystem();
			bus.push16(r.pc);
			r.pc = (opcode - 0xC7);
			return;
		}
	}
}

// Returns a pointer to the register decoded from 3 bits
uint8_t *SM83::decodeReg8(uint8_t opcode_) {
	switch (opcode_ & 0x07) {
	case 0:return &r.b;
	case 1:return &r.c;
	case 2:return &r.d;
	case 3:return &r.e;
	case 4:return &r.h;
	case 5:return &r.l;
	case 6:memOperand8 = fetchByte(r.hl);return &memOperand8;
	case 7:return &r.a;
	default:return nullptr;
	}
}

uint16_t *SM83::decodeReg16(uint8_t opcode_) {
	switch ((opcode_ >> 4) & 0x03) {
	case 0:return &r.bc;
	case 1:return &r.de;
	case 2:return &r.hl;
	case 3:return &r.sp;
	default:return nullptr;
	}
}

bool SM83::checkBranchCondition(uint8_t opcode_) {
	switch ((opcode_ & 0x18) >> 3) {
	case 0:return !r.Z; // NZ
	case 1:return r.Z;  // Z
	case 2:return !r.C; // NC
	case 3:return r.C;  // C
	default:return false;
	}
}

// Reads/writes that take care of timing
uint8_t SM83::fetchByte(uint16_t address) {
	bus.cycleSystem();
	return bus.read8(address);
}

uint16_t SM83::fetchWord(uint16_t address) {
	bus.cycleSystem();
	bus.cycleSystem();
	return bus.read16(address);
}

inline void SM83::writeByte(uint16_t address, uint8_t value) {
	bus.cycleSystem();
	bus.write8(address, value);

	return;
}

void SM83::RLC(uint8_t *operand) {
	setN(0);
	setH(0);
	oldCarry = (*operand & 0x80) >> 7;
	setC(oldCarry);
	*operand = (*operand << 1) | oldCarry;
	testZ(*operand);

	return;
}

void SM83::RRC(uint8_t *operand) {
	setN(0);
	setH(0);
	setC(*operand & 0x01);
	*operand = (*operand >> 1) | (r.C << 7);
	testZ(*operand);

	return;
}

void SM83::RL(uint8_t *operand) {
	setN(0);
	setH(0);
	oldCarry = r.C;
	setC((*operand & 0x80) >> 7);
	*operand = (*operand << 1) | oldCarry;
	testZ(*operand);

	return;
}

void SM83::RR(uint8_t *operand) {
	setN(0);
	setH(0);
	oldCarry = r.C;
	setC(*operand & 0x01);
	*operand = (*operand >> 1) | (oldCarry << 7);
	testZ(*operand);

	return;
}

void SM83::SLA(uint8_t *operand) {
	setN(0);
	setH(0);
	setC((*operand & 0x80) >> 7);
	*operand = *operand << 1;
	testZ(*operand);

	return;
}

void SM83::SRA(uint8_t *operand) {
	setN(0);
	setH(0);
	setC(*operand & 0x01);
	*operand = (*operand >> 1) | (*operand & 0x80);
	testZ(*operand);

	return;
}

void SM83::SWAP(uint8_t *operand) {
	setN(0);
	setH(0);
	setC(0);
	*operand = (*operand << 4) | (*operand >> 4);
	testZ(*operand);

	return;
}

void SM83::SRL(uint8_t *operand) {
	setN(0);
	setH(0);
	setC(*operand & 0x1);
	*operand = *operand >> 1;
	testZ(*operand);
	
	return;
}

void SM83::BIT(uint8_t operand) {
	testZ(operand & (1 << ((cbOpcode >> 3) & 0x07)));
	setN(0);
	setH(1);
}

void SM83::RES(uint8_t *operand) {
	*operand &= ~(1 << ((cbOpcode >> 3) & 0x07));

	return;
}

void SM83::SET(uint8_t *operand) {
	*operand |= (1 << ((cbOpcode >> 3) & 0x07));

	return;
}

void SM83::ADD(uint8_t operand) {
	setN(0);
	testCAdd(r.a, operand, 0);
	testHAdd(r.a, operand, 0);
	r.a += operand;
	testZ(r.a);
	
	return;
}

void SM83::ADC(uint8_t operand) {
	setN(0);
	oldCarry = r.C;
	testCAdd(r.a, operand, oldCarry);
	testHAdd(r.a, operand, oldCarry);
	r.a += (operand + oldCarry);
	testZ(r.a);

	return;
}

void SM83::SUB(uint8_t operand) {
	setN(1);
	testCSub(r.a, operand, 0);
	testHSub(r.a, operand, 0);
	r.a -= operand;
	testZ(r.a);

	return;
}

void SM83::SBC(uint8_t operand) {
	setN(1);
	oldCarry = r.C;
	testCSub(r.a, operand, oldCarry);
	testHSub(r.a, operand, oldCarry);
	r.a -= (operand + oldCarry);
	testZ(r.a);

	return;
}

void SM83::AND(uint8_t operand) {
	setN(0);
	setH(1);
	setC(0);
	r.a &= operand;
	testZ(r.a);

	return;
}
void SM83::XOR(uint8_t operand) {
	setN(0);
	setH(0);
	setC(0);
	r.a ^= operand;
	testZ(r.a);
	
	return;
}

void SM83::OR(uint8_t operand) {
	setN(0);
	setH(0);
	setC(0);
	r.a |= operand;
	testZ(r.a);
	
	return;
}

void SM83::CP(uint8_t operand) {
	setN(1);
	testCSub(r.a, operand, 0);
	testHSub(r.a, operand, 0);
	testZ((uint8_t)(r.a - operand));

	return;
}