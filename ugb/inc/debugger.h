#ifndef __UGB_DEBUGGER_H__
#define __UGB_DEBUGGER_H__

#include "gbm.h"

#include <stdint.h>

typedef struct ugb_breakpoint
{
    int id;
    uint16_t addr;

    struct ugb_breakpoint* prev;
    struct ugb_breakpoint* next;
} ugb_breakpoint;

int ugb_debugger_add_breakpoint(uint16_t addr);
int ugb_debugger_delete_breakpoint(int id);

int ugb_debugger_mainloop(ugb_gbm* gbm);

#endif // __UGB_DEBUGGER_H__
