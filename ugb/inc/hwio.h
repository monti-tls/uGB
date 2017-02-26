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

#ifndef __UGB_HWIO_H__
#define __UGB_HWIO_H__

#include "gbm.h"

enum
{
    #define DEF_HWREG(offset, name, ...) UGB_HWIO_REG_ ## name = 0x ## offset,
    #include "hwio.def"

    UGB_HWIO_REG_SIZE
};

typedef struct ugb_hwreg
{
    const char* name;
    uint8_t wmask;
    uint8_t rmask;
    uint8_t umask;
    uint8_t reset;

    int(*hook)(struct ugb_hwreg*, void*);
    void* cookie;
} ugb_hwreg;

typedef struct ugb_hwio
{
    ugb_gbm* gbm;

    ugb_hwreg regs[UGB_HWIO_REG_SIZE];
    uint8_t data[UGB_HWIO_REG_SIZE];
} ugb_hwio;

ugb_hwio* ugb_hwio_create(ugb_gbm* gbm);
void ugb_hwio_destroy(ugb_hwio* hwio);

int ugb_hwio_reset(ugb_hwio* hwio);
int ugb_hwio_set_hook(ugb_hwio* hwio, uint8_t id, int(*hook)(ugb_hwreg*, void*), void* cookie);

int ugb_hwio_mmu_handler(void* cookie, int op, uint16_t offset, uint8_t* data);

#endif // __UGB_HWIO_H__
