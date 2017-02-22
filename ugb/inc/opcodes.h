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

    int(*microcode)(struct ugb_cpu*, uint8_t[]);
} ugb_opcode;

extern ugb_opcode ugb_opcodes_table[];
extern ugb_opcode ugb_opcodes_tableCB[];

ssize_t ugb_disassemble(char* str, size_t size, uint8_t* code, ssize_t addr);

#endif // __UGB_OPCODES_H__
