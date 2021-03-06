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

#include "hwio.h"
#include "mmu.h"
#include "errno.h"

#include <stdlib.h>
#include <string.h>

ugb_hwio* ugb_hwio_create(ugb_gbm* gbm)
{
    ugb_hwio* hwio = malloc(sizeof(ugb_hwio));
    if (!hwio)
        return 0;

    memset(hwio, 0, sizeof(ugb_hwio));
    hwio->gbm = 0;

    #define DEF_HWREG(offset, name_, reset_, wmask_, rmask_, umask_) \
    hwio->regs[0x ## offset].name = #name_; \
    hwio->regs[0x ## offset].wmask = wmask_; \
    hwio->regs[0x ## offset].rmask = rmask_; \
    hwio->regs[0x ## offset].umask = umask_; \
    hwio->regs[0x ## offset].reset = reset_;
    #include "hwio.def"

    ugb_hwio_reset(hwio);

    return hwio;
}

void ugb_hwio_destroy(ugb_hwio* hwio)
{
    if (hwio)
        free(hwio);
}

int ugb_hwio_reset(ugb_hwio* hwio)
{
    if (!hwio)
        return UGB_ERR_BADARGS;

    for (int i = 0; i < UGB_HWIO_REG_SIZE; ++i)
        hwio->data[i] = hwio->regs[i].reset;

    return UGB_ERR_OK;
}

int ugb_hwio_set_hook(ugb_hwio* hwio, uint8_t id, int(*hook)(ugb_hwreg*, void*), void* cookie)
{
    if (!hwio || id > UGB_HWIO_REG_SIZE || !hwio->regs[id].name)
        return UGB_ERR_BADARGS;

    hwio->regs[id].hook = hook;
    hwio->regs[id].cookie = cookie;

    return UGB_ERR_OK;
}

int ugb_hwio_mmu_handler(void* cookie, int op, uint16_t offset, uint8_t* data)
{
    if (!cookie)
        return UGB_ERR_BADARGS;

    ugb_hwio* hwio = (ugb_hwio*) cookie;
    ugb_hwreg* reg = offset < UGB_HWIO_REG_SIZE && hwio->regs[offset].name ? &hwio->regs[offset] : 0;

    if (!reg)
    {
        if (op == UGB_MMU_READ)
            *data = 0xFF;

        return UGB_ERR_OK;
    }

    switch (op)
    {
        case UGB_MMU_READ:
        {
            *data = (hwio->data[offset] & reg->rmask) | ~reg->rmask | reg->umask;
            break;
        }

        case UGB_MMU_WRITE:
        {
            hwio->data[offset] = (*data) & reg->wmask;

            if (reg->hook)
                (*reg->hook)(reg, reg->cookie);
            break;
        }
    }

    return UGB_ERR_OK;
}
