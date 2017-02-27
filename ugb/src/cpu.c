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
#include "errno.h"

#include <stdlib.h>
#include <string.h>

ugb_cpu* ugb_cpu_create(ugb_gbm* gbm)
{
    if (!gbm)
        return 0;

    ugb_cpu* cpu = malloc(sizeof(ugb_cpu));
    if (!cpu)
        return 0;

    memset(cpu, 0, sizeof(ugb_cpu));
    cpu->gbm = gbm;

    #define DEF_REGB(name, value) cpu->regs.name = (uint8_t*)  &cpu->regs.data[value];
    #define DEF_REGW(name, value) cpu->regs.name = (uint16_t*) &cpu->regs.data[value];
    #include "cpu.def"

    ugb_cpu_reset(cpu);

    return cpu;
}

void ugb_cpu_destroy(ugb_cpu* cpu)
{
    if (cpu)
    {
        free(cpu);
    }
}

ssize_t ugb_cpu_reset(ugb_cpu* cpu)
{
    if (!cpu)
        return UGB_ERR_BADARGS;

    memset(&cpu->regs.data[0], 0, UGB_REGS_SIZE);
    cpu->state = UGB_CPU_RUNNING;
    cpu->ei_delayed = 0;
    cpu->repeat_next_byte = 0;

    return *cpu->regs.PC;
}

ssize_t ugb_cpu_step(ugb_cpu* cpu, size_t* cycles)
{
    int err;

    if (!cpu)
        return UGB_ERR_BADARGS;

    // When stopped, don't do anything
    if (cpu->state == UGB_CPU_STOPPED)
    {
        if (cycles) *cycles = 4;
        return UGB_ERR_OK;
    }

    // Alias to the memory-mapped IF register
    uint8_t* hwreg_if = &cpu->gbm->hwio->data[UGB_HWIO_REG_IF];

    // Exit HALT even if IME == 0
    if (*hwreg_if & *cpu->regs.IE)
    {
        if (cpu->state == UGB_CPU_HALTED)
            printf("Waking up.\n");
        cpu->state = UGB_CPU_RUNNING;
    }

    // Process interrupts if IME == 1
    if (*cpu->regs.IE & UGB_REG_IE_IME_MSK)
    {
        for (int line = 0; line < 5; ++line)
        {
            if (!(*hwreg_if & *cpu->regs.IE & (0x01 << line)))
                continue;

            const char* intname = 0;
            if ((0x01 << line) & UGB_REG_IE_V_MSK) intname = "VBlank";
            if ((0x01 << line) & UGB_REG_IE_L_MSK) intname = "LCD Stat";
            // if ((0x01 << line) & UGB_REG_IE_T_MSK) intname = "Timer";
            if ((0x01 << line) & UGB_REG_IE_X_MSK) intname = "Joypad";
            if (intname)
                printf("Interrupt #%d (%s)\n", line, intname);

            // Clear IME
            *cpu->regs.IE &= ~UGB_REG_IE_IME_MSK;
            cpu->ei_delayed = 0;

            // Clear interrupt flag
            *hwreg_if &= ~(0x01 << line);

            // Push PC
            uint8_t* PC = (uint8_t*) cpu->regs.PC;
            if ((err = ugb_mmu_write(cpu->gbm->mmu, --(*cpu->regs.SP), PC[1])) != UGB_ERR_OK ||
                (err = ugb_mmu_write(cpu->gbm->mmu, --(*cpu->regs.SP), PC[0])) != UGB_ERR_OK)
            {
                return err;
            }

            // Jump to interrupt vector
            *cpu->regs.PC = 0x0040 + (line << 3);

            // Only process one interrupt at a time
            break;
        }
    }

    // Process delayed EI instruction
    if (cpu->ei_delayed)
    {
        *cpu->regs.IE |= UGB_REG_IE_IME_MSK;
        cpu->ei_delayed = 0;
    }

    // If we're halted, do nothing until next step
    if (cpu->state == UGB_CPU_HALTED)
    {
        if (cycles) *(cycles) = 4;
        return UGB_ERR_OK;
    }

    // Decoded instruction will be placed here
    ugb_opcode* opcode = 0;
    uint8_t imm[4];

    // Fetch instruction opcode
    uint8_t op;
    if ((err = ugb_mmu_read(cpu->gbm->mmu, (*cpu->regs.PC)++, &op)) != UGB_ERR_OK)
        return err;

    // Handle HALT bug
    if (cpu->repeat_next_byte)
    {
        --(*cpu->regs.PC);
        cpu->repeat_next_byte = 0;
    }

    // Simple opcode
    if (op != 0xCB)
    {
        // Decode
        opcode = &ugb_opcodes_table[op];
        if (!opcode->microcode)
            return UGB_ERR_BADOP;

        // Get immediate data
        for (int i = 0; i < opcode->size - 1; ++i)
        {
            if ((err = ugb_mmu_read(cpu->gbm->mmu, (*cpu->regs.PC)++, &imm[i])) != UGB_ERR_OK)
                return err;
        }
    }
    // Extended opcode
    else
    {
        // Read actual prefixed opcode
        if ((err = ugb_mmu_read(cpu->gbm->mmu, (*cpu->regs.PC)++, &op)) != UGB_ERR_OK)
                return err;

        // Decode
        opcode = &ugb_opcodes_tableCB[op];
        if (!opcode->microcode)
            return UGB_ERR_BADOP;

        // Get immediate data
        for (int i = 0; i < opcode->size - 2; ++i)
        {
            if ((err = ugb_mmu_read(cpu->gbm->mmu, (*cpu->regs.PC)++, &imm[i])) != UGB_ERR_OK)
                return err;
        }
    }

    // Execute instruction
    if ((err = (*opcode->microcode)(cpu, imm, cycles)) != UGB_ERR_OK)
        return err;

    return UGB_ERR_OK;
}

int ugb_cpu_iereg_mmu_handler(void* cookie, int op, uint16_t offset, uint8_t* data)
{
    if (!cookie || !data || offset != 0)
        return UGB_ERR_BADARGS;

    ugb_cpu* cpu = (ugb_cpu*) cookie;

    switch (op)
    {
        case UGB_MMU_READ:
            // Only expose bits 4-0
            *data = (*cpu->regs.IE) & 0x1F;
            break;

        case UGB_MMU_WRITE:
            // Only expose bits 4-0
            *cpu->regs.IE &= ~0x1F;
            *cpu->regs.IE |= (*data & 0x1F);
            break;
    }

    return UGB_ERR_OK;
}
