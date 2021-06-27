
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
		bus.write16(bus.read16(r.pc), r.sp);
		r.pc += 2;
		bus.cpu.counter += 4;
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
		r.pc += (int8_t)fetchByte(r.pc++);
		bus.cpu.counter += 1;
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
		setZ(r.a?0:1);
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
		r.pc = fetchWord(r.pc);
		bus.cpu.counter += 1;
		return;
	case 0xC9: // RET
		r.pc = bus.pop16();
		bus.cpu.counter += 3;
		return;
	case 0xCD: // CALL imm16
		bus.push16(r.pc + 2);
		r.pc = bus.read16(r.pc);
		bus.cpu.counter += 5;
		return;
	case 0xD9: // RETI
		r.pc = bus.pop16();
		bus.cpu.interruptMasterEnable = true;
		bus.cpu.counter += 3;
		return;
	case 0xE0: // LD (FF00+imm8), A
		writeByte((0xFF00 + fetchByte(r.pc++)), r.a);
		return;
	case 0xE2: // LD (FF00+C), A
		writeByte((0xFF00 + r.c), r.a);
		return;
	case 0xE8: // ADD SP, imm8
		setZ(0);
		setN(0);
		memOperand8 = fetchByte(r.pc++);
		setC((((r.sp & 0xFF) + ((int8_t)memOperand8 & 0xFF)) > 0xFF) ? 1 : 0);
		testHAdd(r.sp, (int8_t)memOperand8, 0);
		r.sp += (int8_t)memOperand8;
		bus.cpu.counter += 2;
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
	case 0xF2: // LD A, (FF00+C)
		r.a = fetchByte(0xFF00 + r.c);
		return;
	case 0xF3: // DI
		bus.cpu.interruptMasterEnable = false;
		return;
	case 0xF8: // LD HL, SP+imm8
		setZ(0);
		setN(0);
		memOperand8 = fetchByte(r.pc++);
		setC((((r.sp & 0xFF) + ((int8_t)memOperand8 & 0xFF)) > 0xFF) ? 1 : 0);
		testHAdd(r.sp, (int8_t)memOperand8, 0);
		r.hl = r.sp + (int8_t)memOperand8;
		bus.cpu.counter += 1;
		return;
	case 0xF9: // LD SP, HL
		r.sp = r.hl;
		bus.cpu.counter += 1;
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
				++*operand16;
				bus.cpu.counter += 1;
				return;
			case 0x9: // ADD HL, reg16
				setN(0);
				setC(((r.hl + *operand16) > 0xFFFF) ? 1 : 0);
				setH((((r.hl & 0xFFF) + (*operand16 & 0xFFF)) > 0x0FFF) ? 1 : 0);
				r.hl += *operand16;
				bus.cpu.counter += 1;
				return;
			case 0xB: // DEC reg16
				--*operand16;
				bus.cpu.counter += 1;
				return;
			}
		}
		switch (opcode & 0x07) {
		case 0: // JR cond, imm8
			memOperand8 = fetchByte(r.pc++);
			if (checkBranchCondition(opcode)) {
				r.pc += (int8_t)memOperand8;
				bus.cpu.counter += 1;
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
			memOperand8 = fetchByte(r.pc++);
			operand8 = &memOperand8;
			goto ops_ld2;
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
			ops_ld2:
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
			if (checkBranchCondition(opcode)) {
				r.pc = bus.pop16();
				bus.cpu.counter += 4;
			} else {
				bus.cpu.counter += 1;
			}
			return;
		case 1: // POP reg16
			((opcode == 0xF1) ? r.af : *decodeReg16(opcode)) = bus.pop16();
			r.f &= 0xF0; // Undo writes to bottom nibble of flags register
			bus.cpu.counter += 2;
			return;
		case 2: // JP cond, imm16
			memOperand16 = fetchWord(r.pc);
			r.pc += 2;
			if (checkBranchCondition(opcode)) {
				r.pc = memOperand16;
				bus.cpu.counter += 1;
			}
			return;
		case 4: // CALL cond, imm16
			memOperand16 = fetchWord(r.pc);
			r.pc += 2;
			if (checkBranchCondition(opcode)) {
				bus.push16(r.pc);
				r.pc = memOperand16;
				bus.cpu.counter += 3;
			}
			return;
		case 5: // PUSH reg16
			bus.push16((opcode == 0xF5) ? r.af : *decodeReg16(opcode));
			bus.cpu.counter += 3;
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
			bus.push16(r.pc);
			r.pc = (opcode - 0xC7);
			bus.cpu.counter += 3;
			return;
		}
	}
}

#define waitForMCycle(x) if (mCycle < x) return
#define endInstruction() printf("0x%02X  %d\n", opcode, mCycle); mCycle = 0; return
//printf("0x%02X  %d\n", opcode, mCycle);

// vvv Spaghetti Part 2: Cycle Accurate Boogaloo vvv
void SM83::executeOpcodeCA() {
	++mCycle;
	if (mCycle == 1)
		opcode = fetchByte(r.pc++);

	if (opcode == 0xCB) { // Prefix CB instructions
		waitForMCycle(2);
		if (mCycle == 2) {
			cbOpcode = fetchByte(r.pc++);
			operand8 = decodeReg8(cbOpcode);
		}

		if ((cbOpcode & 7) != 6) {
			switch (cbOpcode) {
			case 0x00 ... 0x07: // RLC
				RLC(operand8);
				endInstruction();
			case 0x08 ... 0x0F: // RRC reg8
				RRC(operand8);
				endInstruction();
			case 0x10 ... 0x17: // RL reg8
				RL(operand8);
				endInstruction();
			case 0x18 ... 0x1F: // RR reg8
				RR(operand8);
				endInstruction();
			case 0x20 ... 0x27: // SLA reg8
				SLA(operand8);
				endInstruction();
			case 0x28 ... 0x2F: // SRA reg8
				SRA(operand8);
				endInstruction();
			case 0x30 ... 0x37: // SWAP reg8
				SWAP(operand8);
				endInstruction();
			case 0x38 ... 0x3F: // SRL reg8
				SRL(operand8);
				endInstruction();
			case 0x40 ... 0x7F: // BIT bit, reg8
				BIT(*operand8);
				endInstruction();
			case 0x80 ... 0xBF: // RES bit, reg8
				RES(operand8);
				endInstruction();
			case 0xC0 ... 0xFF: // SET bit, reg8
				SET(operand8);
				endInstruction();
			}
		} else { // (HL) as operand
			switch (cbOpcode) {
			case 0x06: // RLC
				switch (mCycle) {
				case 2:return;
				case 3:
					memOperand8 = fetchByte(r.hl);
					RLC(&memOperand8);
					return;
				case 4:
					writeByte(r.hl, memOperand8);
					endInstruction();
				}
			case 0x0E: // RRC reg8
				switch (mCycle) {
				case 2:return;
				case 3:
					memOperand8 = fetchByte(r.hl);
					RRC(&memOperand8);
					return;
				case 4:
					writeByte(r.hl, memOperand8);
					endInstruction();
				}
			case 0x16: // RL reg8
				switch (mCycle) {
				case 2:return;
				case 3:
					memOperand8 = fetchByte(r.hl);
					RL(&memOperand8);
					return;
				case 4:
					writeByte(r.hl, memOperand8);
					endInstruction();
				}
			case 0x1E: // RR reg8
				switch (mCycle) {
				case 2:return;
				case 3:
					memOperand8 = fetchByte(r.hl);
					RR(&memOperand8);
					return;
				case 4:
					writeByte(r.hl, memOperand8);
					endInstruction();
				}
			case 0x26: // SLA reg8
				switch (mCycle) {
				case 2:return;
				case 3:
					memOperand8 = fetchByte(r.hl);
					SLA(&memOperand8);
					return;
				case 4:
					writeByte(r.hl, memOperand8);
					endInstruction();
				}
			case 0x2E: // SRA reg8
				switch (mCycle) {
				case 2:return;
				case 3:
					memOperand8 = fetchByte(r.hl);
					SRA(&memOperand8);
					return;
				case 4:
					writeByte(r.hl, memOperand8);
					endInstruction();
				}
			case 0x36: // SWAP reg8
				switch (mCycle) {
				case 2:return;
				case 3:
					memOperand8 = fetchByte(r.hl);
					SWAP(&memOperand8);
					return;
				case 4:
					writeByte(r.hl, memOperand8);
					endInstruction();
				}
			case 0x3E: // SRL reg8
				switch (mCycle) {
				case 2:return;
				case 3:
					memOperand8 = fetchByte(r.hl);
					SRL(&memOperand8);
					return;
				case 4:
					writeByte(r.hl, memOperand8);
					endInstruction();
				}
			case 0x40 ... 0x7F: // BIT bit, reg8
				switch (mCycle) {
				case 2:return;
				case 3:
					memOperand8 = fetchByte(r.hl);
					BIT(memOperand8);
					endInstruction();
				}
			case 0x80 ... 0xBF: // RES bit, reg8
				switch (mCycle) {
				case 2:return;
				case 3:
					memOperand8 = fetchByte(r.hl);
					RES(&memOperand8);
					return;
				case 4:
					writeByte(r.hl, memOperand8);
					endInstruction();
				}
			case 0xC0 ... 0xFF: // SET bit, reg8
				switch (mCycle) {
				case 2:return;
				case 3:
					memOperand8 = fetchByte(r.hl);
					SET(&memOperand8);
					return;
				case 4:
					writeByte(r.hl, memOperand8);
					endInstruction();
				}
			}
		}
	}

	// Take care of instructions with only one opcode or that don't have a pattern
	switch (opcode) {
	case 0x00: // NOP
		endInstruction();
	case 0x02: // LD (BC), A
		waitForMCycle(2);
		writeByte(r.bc, r.a);
		endInstruction();
	case 0x07: // RLCA
		RLC(&r.a);
		setZ(0);
		endInstruction();
	case 0x08: // LD (imm16), SP
		switch (mCycle) {
		case 1:return;
		case 2:
			memOperand16 = fetchByte(r.pc++);
			return;
		case 3:
			memOperand16 |= fetchByte(r.pc++) << 8;
			return;
		case 4:
			writeByte(memOperand16, (r.sp & 0xFF));
			return;
		case 5:
			writeByte(memOperand16 + 1, (r.sp >> 8));
			endInstruction();
		}
	case 0x0A: // LD A, (BC)
		waitForMCycle(2);
		r.a = fetchByte(r.bc);
		endInstruction();
	case 0x0F: // RRCA
		RRC(&r.a);
		setZ(0);
		endInstruction();
	case 0x10: // STOP
		bus.cpu.stopped = true;
		++r.pc;
		endInstruction();
	case 0x12: // LD (DE), A
		waitForMCycle(2);
		writeByte(r.de, r.a);
		endInstruction();
	case 0x17: // RLA
		RL(&r.a);
		setZ(0);
		endInstruction();
	case 0x18: // JR imm8
		switch (mCycle) {
		case 1:return;
		case 2:
			memOperand8 = fetchByte(r.pc++);
			return;
		case 3:
			r.pc += (int8_t)memOperand8;
			bus.cpu.counter += 1;
			endInstruction();
		}
	case 0x1A: // LD A, (DE)
		waitForMCycle(2);
		r.a = fetchByte(r.de);
		endInstruction();
	case 0x1F: // RRA
		RR(&r.a);
		setZ(0);
		endInstruction();
	case 0x22: // LD (HL+), A
		waitForMCycle(2);
		writeByte(r.hl++, r.a);
		endInstruction();
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
		setZ(r.a ? 0 : 1);
		setH(0);
		endInstruction();
	case 0x2A: // LD A, (HL+)
		waitForMCycle(2);
		r.a = fetchByte(r.hl++);
		endInstruction();
	case 0x2F: // CPL
		setN(1);
		setH(1);
		r.a = ~r.a;
		endInstruction();
	case 0x32: // LD (HL-), A
		waitForMCycle(2);
		writeByte(r.hl--, r.a);
		endInstruction();
	case 0x34: // INC (HL)
		switch (mCycle) {
		case 1:return;
		case 2:
			memOperand8 = fetchByte(r.hl);
			setN(0);
			testHAdd(memOperand8, 1, 0);
			testZ(++memOperand8);
			return;
		case 3:
			writeByte(r.hl, memOperand8);
			endInstruction();
		}
	case 0x35: // DEC (HL)
		switch (mCycle) {
		case 1:return;
		case 2:
			memOperand8 = fetchByte(r.hl);
			setN(1);
			testHSub(memOperand8, 1, 0);
			testZ(--memOperand8);
			return;
		case 3:
			writeByte(r.hl, memOperand8);
			endInstruction();
		}
	case 0x36: // LD (HL), imm8
		switch (mCycle) {
		case 1:return;
		case 2:
			memOperand8 = fetchByte(r.pc++);
			return;
		case 3:
			writeByte(r.hl, memOperand8);
			endInstruction();
		}
	case 0x37: // SCF
		setN(0);
		setH(0);
		setC(1);
		endInstruction();
	case 0x3A: // LD A, (HL-)
		waitForMCycle(2);
		r.a = fetchByte(r.hl--);
		endInstruction();
	case 0x3F: // CCF
		setN(0);
		setH(0);
		setC(~r.C);
		endInstruction();
	case 0x76: // HALT
		bus.cpu.halted = true;
		endInstruction();
	case 0xC3: // JP imm16
		switch (mCycle) {
		case 1:return;
		case 2:
			memOperand16 = fetchByte(r.pc);
			return;
		case 3:
			memOperand16 |= fetchByte(r.pc + 1) << 8;
			return;
		case 4:
			r.pc = memOperand16;
			bus.cpu.counter += 1;
			endInstruction();
		}
	case 0xC9: // RET
		switch (mCycle) {
		case 1:return;
		case 2:
			memOperand16 = fetchByte(r.sp++);
			return;
		case 3:
			memOperand16 |= fetchByte(r.sp++) << 8;
			return;
		case 4:
			r.pc = memOperand16;
			bus.cpu.counter += 1;
			endInstruction();
		}
	case 0xCD: // CALL imm16
		switch (mCycle) {
		case 1:return;
		case 2:
			memOperand16 = fetchByte(r.pc++);
			return;
		case 3:
			memOperand16 |= fetchByte(r.pc++) << 8;
			return;
		case 4:
			bus.cpu.counter += 1;
			return;
		case 5:
			writeByte(--r.sp, r.pc >> 8);
			return;
		case 6:
			writeByte(--r.sp, (r.pc & 0xFF));
			r.pc = memOperand16;
			endInstruction();
		}
	case 0xD9: // RETI
		switch (mCycle) {
		case 1:return;
		case 2:
			memOperand16 = fetchByte(r.sp++);
			return;
		case 3:
			memOperand16 |= fetchByte(r.sp++) << 8;
			return;
		case 4:
			r.pc = memOperand16;
			bus.cpu.interruptMasterEnable = true;
			bus.cpu.counter += 1;
			endInstruction();
		}
	case 0xE0: // LD (FF00+imm8), A
		switch (mCycle) {
		case 1:return;
		case 2:
			memOperand8 = fetchByte(r.pc++);
			return;
		case 3:
			writeByte((0xFF00 + memOperand8), r.a);
			endInstruction();
		}
	case 0xE2: // LD (FF00+C), A
		waitForMCycle(2);
		writeByte((0xFF00 + r.c), r.a);
		endInstruction();
	case 0xE8: // ADD SP, imm8
		switch (mCycle) {
		case 1:return;
		case 2:
			setZ(0);
			setN(0);
			memOperand8 = fetchByte(r.pc++);
			setC((((r.sp & 0xFF) + ((int8_t)memOperand8 & 0xFF)) > 0xFF) ? 1 : 0);
			testHAdd(r.sp, (int8_t)memOperand8, 0);
			memOperand16 = r.sp + (int8_t)memOperand8;
			return;
		case 3:
			r.sp = memOperand16 & 0xFF;
			bus.cpu.counter += 1;
			return;
		case 4:
			r.sp |= memOperand16 & 0xFF00;
			bus.cpu.counter += 1;
			endInstruction();
		}
	case 0xE9: // JP HL
		r.pc = r.hl;
		endInstruction();
	case 0xEA: // LD (imm16), A
		switch (mCycle) {
		case 1:return;
		case 2:
			memOperand16 = fetchByte(r.pc++);
			return;
		case 3:
			memOperand16 |= fetchByte(r.pc++) << 8;
			return;
		case 4:
			writeByte(memOperand16, r.a);
			endInstruction();
		}
	case 0xF0: // LD A, (FF00+imm8)
		switch (mCycle) {
		case 1:return;
		case 2:
			memOperand8 = fetchByte(r.pc++);
			return;
		case 3:
			r.a = fetchByte(0xFF00 + memOperand8);
			endInstruction();
		}
	case 0xF1: // POP AF
		switch (mCycle) {
		case 1:return;
		case 2:
			r.af = fetchByte(r.sp++);
			return;
		case 3:
			r.af |= fetchByte(r.sp++) << 8;
			r.f &= 0xF0; // Undo writes to bottom nibble of flags register
			endInstruction();
		}
	case 0xF2: // LD A, (FF00+C)
		waitForMCycle(2);
		r.a = fetchByte(0xFF00 + r.c);
		endInstruction();
	case 0xF3: // DI
		bus.cpu.interruptMasterEnable = false;
		endInstruction();
	case 0xF5: // PUSH AF
		switch (mCycle) {
		case 1:return;
		case 2:
			bus.cpu.counter += 1;
			return;
		case 3:
			writeByte(--r.sp, r.af >> 8);
			return;
		case 4:
			writeByte(--r.sp, r.af & 0xFF);
			endInstruction();
		}
	case 0xF8: // LD HL, SP+imm8
		switch (mCycle) {
		case 1:return;
		case 2:
			setZ(0);
			setN(0);
			memOperand8 = fetchByte(r.pc++);
			setC((((r.sp & 0xFF) + ((int8_t)memOperand8 & 0xFF)) > 0xFF) ? 1 : 0);
			testHAdd(r.sp, (int8_t)memOperand8, 0);
			r.hl = r.sp + (int8_t)memOperand8;
			return;
		case 3:
			bus.cpu.counter += 1;
			endInstruction();
		}
	case 0xF9: // LD SP, HL
		switch (mCycle) {
		case 1:return;
		case 2:
			r.sp = r.hl;
			return;
		case 3:
			bus.cpu.counter += 1;
			endInstruction();
		}
	case 0xFA: // LD A, (imm16)
		switch (mCycle) {
		case 1:return;
		case 2:
			memOperand16 = fetchByte(r.pc++);
			return;
		case 3:
			memOperand16 |= fetchByte(r.pc++) << 8;
			return;
		case 4:
			r.a = fetchByte(memOperand16);
			endInstruction();
		}
	case 0xFB: // EI
		bus.cpu.interruptMasterEnable = true;
		endInstruction();
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
		endInstruction();
	}

	if (opcode <= 0x3F) {
		switch (opcode & 0x07) {
		case 0: // JR cond, imm8
			switch (mCycle) {
			case 1:return;
			case 2:
				memOperand8 = fetchByte(r.pc++);
				if (!checkBranchCondition(opcode)) {
					endInstruction();
				}
				return;
			case 3:
				r.pc += (int8_t)memOperand8;
				bus.cpu.counter += 1;
				endInstruction();
			}
		case 1:
		case 2:
		case 3:
			if (mCycle == 1) {
				operand16 = decodeReg16(opcode);
				return;
			}
			switch (opcode & 0x0F) {
			case 0x1: // LD reg16, u16
				switch (mCycle) {
				case 2:
					*operand16 = fetchByte(r.pc++);
					return;
				case 3:
					*operand16 |= fetchByte(r.pc++) << 8;
					endInstruction();
				}
			case 0x3: // INC reg16
				++*operand16;
				bus.cpu.counter += 1;
				endInstruction();
			case 0x9: // ADD HL, reg16
				setN(0);
				setC(((r.hl + *operand16) > 0xFFFF) ? 1 : 0);
				setH((((r.hl & 0xFFF) + (*operand16 & 0xFFF)) > 0x0FFF) ? 1 : 0);
				r.hl += *operand16;
				bus.cpu.counter += 1;
				endInstruction();
			case 0xB: // DEC reg16
				--*operand16;
				bus.cpu.counter += 1;
				endInstruction();
			}
		case 4: // INC reg8
			operand8 = decodeReg8(opcode >> 3);
			setN(0);
			testHAdd(*operand8, 1, 0);
			testZ(++*operand8);
			endInstruction();
		case 5: // DEC reg8
			operand8 = decodeReg8(opcode >> 3);
			setN(1);
			testHSub(*operand8, 1, 0);
			testZ(--*operand8);
			endInstruction();
		case 6: // LD reg8, imm8
			waitForMCycle(2);
			*decodeReg8(opcode >> 3) = fetchByte(r.pc++);
			endInstruction();
		}
	}

	if ((opcode >= 0x40) && (opcode <= 0xBF)) { // All take the same operand
		operand8 = decodeReg8(opcode);
		if (opcode <= 0x7F) { // LD instructions
			if ((opcode >= 0x70) && (opcode <= 0x77)) { // LD (HL), reg8
				waitForMCycle(2);
				writeByte(r.hl, *operand8);
				endInstruction();
			}
			// LD reg8, reg8
			*decodeReg8(opcode >> 3) = *operand8;
			endInstruction();
		}
		// 8-bit ALU operations
		if ((opcode & 0x07) == 6) { // (HL) as operand
			waitForMCycle(2);
			memOperand8 = fetchByte(r.hl);
			operand8 = &memOperand8;
		}
		switch (opcode & 0xF8) {
		case 0x80: // ADD A, reg8
			ADD(*operand8);
			break;
		case 0x88: // ADC A, reg8
			ADC(*operand8);
			break;
		case 0x90: // SUB A, reg8
			SUB(*operand8);
			break;
		case 0x98: // SBC A, reg8
			SBC(*operand8);
			break;
		case 0xA0: // AND A, reg8
			AND(*operand8);
			break;
		case 0xA8: // XOR A, reg8
			XOR(*operand8);
			break;
		case 0xB0: // OR A, reg8
			OR(*operand8);
			break;
		case 0xB8: // CP A, reg8
			CP(*operand8);
			break;
		}
		endInstruction();
	}

	if (opcode >= 0xC0) {
		switch (opcode & 0x07) {
		case 0: // RET cond
			switch (mCycle) {
			case 1:return;
			case 2:
				bus.cpu.counter += 1;
				if (!checkBranchCondition(opcode)) {
					endInstruction();
				}
				return;
			case 3:
				memOperand16 = fetchByte(r.sp++);
				return;
			case 4:
				memOperand16 |= fetchByte(r.sp++) << 8;
				return;
			case 5:
				r.pc = memOperand16;
				bus.cpu.counter += 1;
				endInstruction();
			}
		case 1: // POP reg16
			switch (mCycle) {
			case 1:return;
			case 2:
				operand16 = decodeReg16(opcode);
				*operand16 = fetchByte(r.sp++);
				return;
			case 3:
				*operand16 |= fetchByte(r.sp++) << 8;
				endInstruction();
			}
		case 2: // JP cond, imm16
			switch (mCycle) {
			case 1:return;
			case 2:
				memOperand16 = fetchByte(r.pc++);
				return;
			case 3:
				memOperand16 |= fetchByte(r.pc++) << 8;
				if (!checkBranchCondition(opcode)) {
					endInstruction();
				}
				return;
			case 4:
				r.pc = memOperand16;
				bus.cpu.counter += 1;
				endInstruction();
			}
		case 4: // CALL cond, imm16
			switch (mCycle) {
			case 1:return;
			case 2:
				memOperand16 = fetchByte(r.pc++);
				return;
			case 3:
				memOperand16 |= fetchByte(r.pc++) << 8;
				if (!checkBranchCondition(opcode)) {
					endInstruction();
				}
				return;
			case 4:
				bus.cpu.counter += 1;
				return;
			case 5:
				writeByte(--r.sp, r.pc >> 8);
				return;
			case 6:
				writeByte(--r.sp, r.pc & 0xFF);
				r.pc = memOperand16;
				endInstruction();
			}
		case 5: // PUSH reg16
			switch (mCycle) {
			case 1:return;
			case 2:
				bus.cpu.counter += 1;
				return;
			case 3:
				operand16 = decodeReg16(opcode);
				writeByte(--r.sp, *operand16 >> 8);
				return;
			case 4:
				writeByte(--r.sp, *operand16 & 0xFF);
				endInstruction();
			}
		case 6: // ALU A, imm8
			waitForMCycle(2);
			memOperand8 = fetchByte(r.pc++);
			switch (opcode) {
			case 0xC6:ADD(memOperand8);break;
			case 0xCE:ADC(memOperand8);break;
			case 0xD6:SUB(memOperand8);break;
			case 0xDE:SBC(memOperand8);break;
			case 0xE6:AND(memOperand8);break;
			case 0xEE:XOR(memOperand8);break;
			case 0xF6:OR(memOperand8);break;
			case 0xFE:CP(memOperand8);break;
			}
			endInstruction();
		case 7: // RST vector
			switch (mCycle) {
			case 1:return;
			case 2:
				bus.cpu.counter += 1;
				return;
			case 3:
				writeByte(--r.sp, r.pc >> 8);
				return;
			case 4:
				writeByte(--r.sp, r.pc & 0xFF);
				r.pc = (opcode - 0xC7);
				endInstruction();
			}
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
	case 0:return (r.Z ? false : true); // NZ
	case 1:return (r.Z ? true : false); // Z
	case 2:return (r.C ? false : true); // NC
	case 3:return (r.C ? true : false); // C
	default:return false;
	}
}

// Reads/writes that take care of timing
uint8_t SM83::fetchByte(uint16_t address) {
	bus.cpu.counter += 1;
	return bus.read8(address);
}

uint16_t SM83::fetchWord(uint16_t address) {
	bus.cpu.counter += 2;
	return bus.read16(address);
}

inline void SM83::writeByte(uint16_t address, uint8_t value) {
	bus.cpu.counter += 1;
	return bus.write8(address, value);
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