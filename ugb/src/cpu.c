#include "cpu.h"
#include "mmu.h"
#include "opcodes.h"
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

ssize_t ugb_cpu_step(ugb_cpu* cpu)
{
    int err;

    if (!cpu)
        return UGB_ERR_BADARGS;

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
    if ((err = (*opcode->microcode)(cpu, imm)) != UGB_ERR_OK)
        return err;

    //TODO: manage cycles

    return *cpu->regs.PC;
}
