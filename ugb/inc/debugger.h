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
