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

#ifndef __UGB_CPU_H__
#define __UGB_CPU_H__

#include "gbm.h"

#include <stdint.h>
#include <unistd.h>

enum
{
    #define DEF_REGB(name, value) UGB_REG_ ## name = (value),
    #define DEF_REGW(name, value) UGB_REG_ ## name = (value),
    #include "cpu.def"

    UGB_REGS_SIZE,

    #define DEF_REG_MSK(reg, name, bit) UGB_REG_ ## reg ## _ ## name ## _MSK = (0x01 << (bit)),
    #include "cpu.def"

    #define DEF_REG_MSK(reg, name, bit) UGB_REG_ ## reg ## _ ## name ## _BIT = (bit),
    #include "cpu.def"
};

enum
{
    UGB_CPU_RUNNING,
    UGB_CPU_HALTED,
    UGB_CPU_STOPPED
};

typedef struct ugb_cpu
{
    ugb_gbm* gbm;

    struct
    {
        uint8_t data[UGB_REGS_SIZE];

        #define DEF_REGB(name, value) uint8_t* name;
        #define DEF_REGW(name, value) uint16_t* name;
        #include "cpu.def"
    } regs;

    int state;
    int ei_delayed;
    int repeat_next_byte;
} ugb_cpu;

ugb_cpu* ugb_cpu_create(ugb_gbm* gbm);
void ugb_cpu_destroy(ugb_cpu* cpu);

ssize_t ugb_cpu_reset(ugb_cpu* cpu);
ssize_t ugb_cpu_step(ugb_cpu* cpu, size_t* ticks);

// Handler for Interrupt Enable memory-mapped register
int ugb_cpu_iereg_mmu_handler(void* cookie, int op, uint16_t offset, uint8_t* data);

#endif // __UGB_CPU_H__
