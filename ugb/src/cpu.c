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

    return *cpu->regs.PC;
}

ssize_t ugb_cpu_step(ugb_cpu* cpu, size_t* cycles)
{
    int err;

    if (!cpu)
        return UGB_ERR_BADARGS;

    // If enabled, process eventual interrupts
    if (*cpu->regs.IE & UGB_REG_IE_IME_MSK)
    {
        // Get the memory-mapped IF register
        uint8_t* hwreg_if = &cpu->gbm->hwio->data[UGB_HWIO_REG_IF];

        // Check interrupt flags by priority
        for (int i = 0; i <= 4; ++i)
        {
            // If interrupt is pending and unmasked
            if ((*cpu->regs.IE & (0x01 << i)) && (*hwreg_if & (0x01 << i)))
            {
                // Reset interrupt flag
                *hwreg_if = 0;

                // Reset IME flag
                *cpu->regs.IE &= ~UGB_REG_IE_IME_MSK;

                // Push PC
                uint8_t* PC = (uint8_t*) cpu->regs.PC;
                if ((err = ugb_mmu_write(cpu->gbm->mmu, (*cpu->regs.SP)-1, PC[1])) != UGB_ERR_OK ||
                    (err = ugb_mmu_write(cpu->gbm->mmu, (*cpu->regs.SP)-2, PC[0])) != UGB_ERR_OK)
                {
                    return err;
                }

                *cpu->regs.SP -= 2;

                // Jump to interrupt vector
                *cpu->regs.PC = 0x0040 + i * 8;
            }
        }
    }

    // Fetch instruction opcode (TODO: handle CB prefix)
    uint8_t op;
    if ((err = ugb_mmu_read(cpu->gbm->mmu, *cpu->regs.PC, &op)) != UGB_ERR_OK)
        return err;

    ugb_opcode* opcode = 0;
    uint8_t imm[4];

    if (op == 0xCB)
    {
        if ((err = ugb_mmu_read(cpu->gbm->mmu, *cpu->regs.PC + 1, &op)) != UGB_ERR_OK)
            return err;

        // Resolve the instruction
        opcode = &ugb_opcodes_tableCB[op];
        if (!opcode->microcode)
            return UGB_ERR_BADOP;

        // Get immediate operands if applicable
        for (int i = 0; i < opcode->size - 2; ++i)
        {
            if ((err = ugb_mmu_read(cpu->gbm->mmu, *cpu->regs.PC + 2 + i, &imm[i])) != UGB_ERR_OK)
                return err;
        }
    }
    else
    {
        // Resolve the instruction
        opcode = &ugb_opcodes_table[op];
        if (!opcode->microcode)
            return UGB_ERR_BADOP;

        // Get immediate operands if applicable
        for (int i = 0; i < opcode->size - 1; ++i)
        {
            if ((err = ugb_mmu_read(cpu->gbm->mmu, *cpu->regs.PC + 1 + i, &imm[i])) != UGB_ERR_OK)
                return err;
        }
    }

    // Advance PC
    *cpu->regs.PC += opcode->size;

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
