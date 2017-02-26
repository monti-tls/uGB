/*
 * This file is part of uGB
 * Copyright (C) 2017  Alexandre Monti
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cpu.h"
#include "mmu.h"
#include "opcodes.h"
#include "hwio.h"
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

#define DEF_OPCODE(prefix, opcode, ...) extern int _ugb_opcode ## prefix ## opcode(ugb_cpu*, uint8_t[], size_t*);
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

// F flags shorcuts
#define _fZ ((F & UGB_REG_F_Z_MSK) >> UGB_REG_F_Z_BIT)
#define _fN ((F & UGB_REG_F_N_MSK) >> UGB_REG_F_N_BIT)
#define _fH ((F & UGB_REG_F_H_MSK) >> UGB_REG_F_H_BIT)
#define _fC ((F & UGB_REG_F_C_MSK) >> UGB_REG_F_C_BIT)

// Memory read
#define r(addr, data) do { if ((err = ugb_mmu_read(cpu->gbm->mmu, (addr), (data))) < 0) return err; } while (0);
// Memory write
#define w(addr, data) do { if ((err = ugb_mmu_write(cpu->gbm->mmu, (addr), (data))) < 0) return err; } while (0);

#define _Za(x) do { \
    if (x) F |= UGB_REG_F_Z_MSK; \
    else   F &= ~UGB_REG_F_Z_MSK; \
} while (0);

#define _Ha(x) do { \
    if (x) F |= UGB_REG_F_H_MSK; \
    else   F &= ~UGB_REG_F_H_MSK; \
} while (0);

#define _Ca(x) do { \
    if (x) F |= UGB_REG_F_C_MSK; \
    else   F &= ~UGB_REG_F_C_MSK; \
} while (0);

#define _Zv(x) do { \
    if (!(x)) F |= UGB_REG_F_Z_MSK; \
    else      F &= ~UGB_REG_F_Z_MSK; \
} while (0);

// Conditionals
#define _IF(cond, code, overhead) do { if ((cond)) { code; _cycles += (overhead); } } while (0);

// Interrupt masking
#define _EI() do { \
    cpu->ei_delayed = 1; \
} while (0);

#define _DI() do { \
    cpu->ei_delayed = 0; \
    IE &= ~UGB_REG_IE_IME_MSK; \
} while (0);

extern uint16_t mednafen_daa_lookup[];

static void _ugb_opcode_update_flags(ugb_cpu* cpu, char flags[])
{
    static uint8_t msk[4] = {
        UGB_REG_F_Z_MSK,
        UGB_REG_F_N_MSK,
        UGB_REG_F_H_MSK,
        UGB_REG_F_C_MSK
    };

    for (int i = 0; i < 4; ++i)
    {
        if (flags[i] == '0') F &= ~msk[i];
        else if (flags[i] == '1') F |= msk[i];
    }

    F &= 0xF0;
}

#define DEF_OPCODE(prefix, opcode, size, cycles, flags, mnemonic, microcode)\
int _ugb_opcode ## prefix ## opcode(ugb_cpu* cpu, uint8_t imm[], size_t* cycles_counter) \
{ \
    int __attribute__((unused)) err = 0; \
    uint8_t __attribute__((unused)) t8 = 0; \
    uint8_t __attribute__((unused)) t8_ = 0; \
    uint16_t __attribute__((unused)) t16 = 0; \
    uint16_t __attribute__((unused)) t16_ = 0; \
    uint16_t __attribute__((unused)) v16_ = 0; \
    uint32_t __attribute__((unused)) t32_ = 0; \
    size_t _cycles = cycles; \
    microcode; \
    if (cycles_counter) *cycles_counter = _cycles; \
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
