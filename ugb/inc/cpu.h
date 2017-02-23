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
} ugb_cpu;

ugb_cpu* ugb_cpu_create(ugb_gbm* gbm);
void ugb_cpu_destroy(ugb_cpu* cpu);

ssize_t ugb_cpu_reset(ugb_cpu* cpu);
ssize_t ugb_cpu_step(ugb_cpu* cpu, size_t* ticks);

// Handler for Interrupt Enable memory-mapped register
int ugb_cpu_ie_reg_mmu_handler(void* cookie, int op, uint16_t offset, uint8_t* data);

#endif // __UGB_CPU_H__
