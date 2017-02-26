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

#ifndef __UGB_OPCODES_H__
#define __UGB_OPCODES_H__

#include <stdint.h>
#include <unistd.h>

struct ugb_cpu;

typedef struct ugb_opcode
{
    uint16_t code;
    const char* mnemonic;
    uint8_t size;
    uint8_t cycles;
    const char* flags;

    int(*microcode)(struct ugb_cpu*, uint8_t[], size_t*);
} ugb_opcode;

extern ugb_opcode ugb_opcodes_table[];
extern ugb_opcode ugb_opcodes_tableCB[];

ssize_t ugb_read_opcode(uint8_t* buf, ugb_gbm* gbm, uint16_t addr);
ssize_t ugb_disassemble(char* str, size_t size, uint8_t* code, ssize_t addr);

#endif // __UGB_OPCODES_H__
