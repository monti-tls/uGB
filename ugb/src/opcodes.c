#include "cpu.h"
#include "mmu.h"
#include "opcodes.h"
#include "gbm.h"
#include "errno.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/******************************/
/*** Disassembler functions ***/
/******************************/

ssize_t ugb_read_opcode(uint8_t* buf, ugb_gbm* gbm, uint16_t addr)
{
    if (!buf || !gbm)
        return UGB_ERR_BADARGS;

    int err = ugb_mmu_read(gbm->mmu, addr, &buf[0]);
    if (err != UGB_ERR_OK)
        return err;

    ugb_opcode* opcode = 0;

    if (buf[0] == 0xCB)
    {
        if ((err = ugb_mmu_read(gbm->mmu, addr+1, &buf[1])) != UGB_ERR_OK)
            return err;

        opcode = &ugb_opcodes_tableCB[buf[1]];

        for (int i = 0; i < opcode->size-2; ++i)
        {
            if ((err = ugb_mmu_read(gbm->mmu, addr+2+i, &buf[2+i])) != UGB_ERR_OK)
                return err;
        }
    }
    else
    {
        opcode = &ugb_opcodes_table[buf[0]];

        for (int i = 0; i < opcode->size-1; ++i)
        {
            if ((err = ugb_mmu_read(gbm->mmu, addr+1+i, &buf[1+i])) != UGB_ERR_OK)
                return err;
        }
    }

    if (!opcode->microcode)
        return UGB_ERR_BADOP;

    return opcode->size;
}

ssize_t ugb_disassemble(char* str, size_t size, uint8_t* code, ssize_t addr)
{
    if (!code)
        return UGB_ERR_BADARGS;

    ugb_opcode* opcode = 0;

    if (code[0] == 0xCB)
        opcode = &ugb_opcodes_tableCB[code[1]];
    else
        opcode = &ugb_opcodes_table[code[0]];

    if (!opcode->microcode)
        return UGB_ERR_BADOP;

    if (str)
    {
        size_t str_pos = 0;
        for (const char* s = opcode->mnemonic; *s && str_pos < size; )
        {
            if (!strncmp("d8", s, 2))
            {
                str_pos += snprintf(str + str_pos, size - str_pos, "$%02X", code[1]);
                s += 2;
            }
            else if (!strncmp("a8", s, 2))
            {
                str_pos += snprintf(str + str_pos, size - str_pos, "$FF00+%02X", code[1]);
                s += 2;
            }
            else if (!strncmp("r8", s, 2))
            {
                str_pos += snprintf(str + str_pos, size - str_pos, "%+d", *((int8_t*) &code[1]));
                if (addr >= 0)
                    str_pos += snprintf(str + str_pos, size - str_pos, " = %04X", (uint16_t) addr + opcode->size + *((int8_t*) &code[1]));
                s += 2;
            }
            else if (!strncmp("d16", s, 3) ||
                     !strncmp("a16", s, 3))
            {
                str_pos += snprintf(str + str_pos, size - str_pos, "$%04X", *((uint16_t*) &code[1]));
                s += 3;
            }
            else
            {
                str_pos += snprintf(str + str_pos, size - str_pos, "%c", *s);
                ++s;
            }
        }
    }

    return opcode->size;
}

/*************************************/
/*** Define external opcode tables ***/
/*************************************/

ugb_opcode ugb_opcodes_table[0x100];
ugb_opcode ugb_opcodes_tableCB[0x100];

#define DEF_OPCODE(prefix, opcode, ...) extern int _ugb_opcode ## prefix ## opcode(ugb_cpu*, uint8_t[]);
#include "opcodes.def"

static void __attribute__((constructor)) _ugb_opcodes_init()
{
#define DEF_OPCODE(prefix_, opcode_, size_, cycles_, flags_, mnemonic_, ...) \
    ugb_opcodes_table ## prefix_[0x ## opcode_].code = 0x ## prefix_ ## opcode_; \
    ugb_opcodes_table ## prefix_[0x ## opcode_].mnemonic = mnemonic_; \
    ugb_opcodes_table ## prefix_[0x ## opcode_].size = size_; \
    ugb_opcodes_table ## prefix_[0x ## opcode_].cycles = cycles_; \
    ugb_opcodes_table ## prefix_[0x ## opcode_].flags = flags_; \
    ugb_opcodes_table ## prefix_[0x ## opcode_].microcode = &_ugb_opcode ## prefix_ ## opcode_;
#include "opcodes.def"
}

/****************************************************/
/*** Define actual opcode implementation routines ***/
/****************************************************/

// Registers shortcuts
#define SP  (*cpu->regs.SP)
#define SPl ((uint8_t*) &SP)[0]
#define SPh ((uint8_t*) &SP)[1]
#define PC  (*cpu->regs.PC)
#define PCl ((uint8_t*) &PC)[0]
#define PCh ((uint8_t*) &PC)[1]
#define IE  (*cpu->regs.IE)
#define AF  (*cpu->regs.AF)
#define AFl ((uint8_t*) &AF)[0]
#define AFh ((uint8_t*) &AF)[1]
#define BC  (*cpu->regs.BC)
#define BCl ((uint8_t*) &BC)[0]
#define BCh ((uint8_t*) &BC)[1]
#define DE  (*cpu->regs.DE)
#define DEl ((uint8_t*) &DE)[0]
#define DEh ((uint8_t*) &DE)[1]
#define HL  (*cpu->regs.HL)
#define HLl ((uint8_t*) &HL)[0]
#define HLh ((uint8_t*) &HL)[1]
#define A   (*cpu->regs.A)
#define F   (*cpu->regs.F)
#define B   (*cpu->regs.B)
#define C   (*cpu->regs.C)
#define D   (*cpu->regs.D)
#define E   (*cpu->regs.E)
#define H   (*cpu->regs.H)
#define L   (*cpu->regs.L)

// Immediate operands shotcuts
#define d8  (*((uint8_t*) &imm[0]))
#define a8  (*((uint8_t*) &imm[0]))
#define r8  (*((int8_t*) &imm[0]))
#define d16 (*((uint16_t*) &imm[0]))
#define a16 (*((uint16_t*) &imm[0]))

// Memory read
#define r(addr, data) do { if ((err = ugb_mmu_read(cpu->gbm->mmu, (addr), (data))) < 0) return err; } while (0);
// Memory write
#define w(addr, data) do { if ((err = ugb_mmu_write(cpu->gbm->mmu, (addr), (data))) < 0) return err; } while (0);

// Rotate left x
#define rol8(x) ((x) >> 7) | ((x) << 1)
// Rotate right x
#define ror8(x) ((x) << 7) | ((x) >> 1)
// Shift left x, bit 0 is 0
#define sl8(x) (((x) << 1) & ~0x01)
// Shift right x, copy bit 7
#define sr8c(x) (((x) >> 1) | ((x) & 0x80))
// Shift right, bit 7 is 0
#define sr8(x) (((x) >> 1) & ~0x01)
// Swap nibbles
#define swap(x) ((((x) >> 4) & 0xF) | (((x) << 4) & 0xF0))

// Set F.Z flag if x is zero
#define _Z(x) do { uint16_t x_ = (x); if (!x_) F |= UGB_REG_F_Z_MSK; } while (0);
// Assign F.Z flag
#define _Za(x) do { uint16_t x_ = (x); if (!x_) F |= UGB_REG_F_Z_MSK; else F &= ~UGB_REG_F_Z_MSK; } while (0);
// Set F.H flag if half carry from bit 3
#define _H3(o, n) do { uint16_t o_ = (o); uint16_t n_ = (n); if (((n_ & 0xF) - (o_ & 0xF)) < 0) F |= UGB_REG_F_H_MSK; } while (0);
// Set F.H flag if half carry from bit 11
#define _H11(o, n) _H3(((o) >> 8), ((n) >> 8))
// Set F.C flag if carry from bit 15
#define _C15(o, n) do { uint16_t o_ = (o); uint16_t n_ = (n); if (n_ - o_ < 0) F |= UGB_REG_F_C_MSK; } while (0);
// Set F.H flag if half carry from bit 11 and F.C flag if carry from bit 15
#define _C15H11(o, n) do { uint16_t o2_ = (o); uint16_t n2_ = (n); _C15(o2_, n2_); _H11(o2_, n2_); } while (0);
// Set F.H flag if half carry from bit 11 and F.C flag if carry from bit 15, set F.Z flag if n zero
#define _ZC15H11(o, n) do { uint16_t n3_ = (n); _C15H11((o), n3_); _Z(n3_); } while (0);
// Set F.C flag if y != 0
#define _Cs(y) do { uint16_t y_ = (y); if (y_ != 0) F |= UGB_REG_F_C_MSK; } while (0);
// Assign F.C flag
#define _Ca(y) do { uint16_t y_ = (y); if (y_ != 0) F |= UGB_REG_F_C_MSK; else F &= ~UGB_REG_F_C_MSK; } while (0);

// F flags shorcuts
#define _fZ ((F & UGB_REG_F_Z_MSK) >> UGB_REG_F_Z_BIT)
#define _fN ((F & UGB_REG_F_N_MSK) >> UGB_REG_F_N_BIT)
#define _fH ((F & UGB_REG_F_H_MSK) >> UGB_REG_F_H_BIT)
#define _fC ((F & UGB_REG_F_C_MSK) >> UGB_REG_F_C_BIT)

// Conditionals
#define _IF(cond, code, overhead) do { if ((cond)) { code; } } while (0);

static void _ugb_opcode_update_flags(ugb_cpu* cpu, char flags[])
{
    static uint16_t msk[4] = {
        UGB_REG_F_Z_MSK,
        UGB_REG_F_N_MSK,
        UGB_REG_F_H_MSK,
        UGB_REG_F_C_MSK,
    };

    for (int i = 0; i < 4; ++i)
    {
        if (flags[i] == '0') F &= ~msk[i];
        else if (flags[i] == '1') F |= msk[i];
    }
}

#define DEF_OPCODE(prefix, opcode, size, cycles, flags, mnemonic, microcode)\
int _ugb_opcode ## prefix ## opcode(ugb_cpu* cpu, uint8_t imm[]) \
{ \
    int __attribute__((unused)) err = 0; \
    uint8_t __attribute__((unused)) t8 = 0; \
    uint16_t __attribute__((unused)) t16 = 0; \
    microcode; \
    _ugb_opcode_update_flags(cpu, (flags)); \
    return UGB_ERR_OK; \
}
#include "opcodes.def"

#undef _IF
#undef _fC
#undef _fH
#undef _fN
#undef _fZ
#undef _Ca
#undef _Cs
#undef _ZC15H11
#undef _C15H11
#undef _C15
#undef _H11
#undef _H3
#undef _Za
#undef _Z
#undef swap
#undef sr8
#undef sr8c
#undef sl8
#undef ror16
#undef rol16
#undef w
#undef r
#undef a16
#undef d16
#undef r8
#undef a8
#undef d8
#undef L
#undef H
#undef E
#undef D
#undef C
#undef B
#undef F
#undef A
#undef HL
#undef DE
#undef DC
#undef AF
#undef IE
#undef PCh
#undef PCl
#undef PC
#undef SPh
#undef SPl
#undef SP
