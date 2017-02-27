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

#include "joypad.h"
#include "gbm.h"
#include "hwio.h"
#include "cpu.h"
#include "constants.h"
#include "errno.h"

#include <stdlib.h>
#include <string.h>

ugb_joypad* ugb_joypad_create(ugb_gbm* gbm)
{
    ugb_joypad* joypad = malloc(sizeof(ugb_joypad));
    if (!joypad)
        return 0;

    memset(joypad, 0, sizeof(ugb_joypad));
    joypad->gbm = gbm;

    return joypad;
}

void ugb_joypad_destroy(ugb_joypad* joypad)
{
    if (joypad)
    {
        free(joypad);
    }
}

int ugb_joypad_reset(ugb_joypad* joypad)
{
    if (!joypad)
        return UGB_ERR_BADARGS;

    return UGB_ERR_OK;
}

int ugb_joypad_press(ugb_joypad* joypad, uint8_t keys)
{
    if (!joypad)
        return UGB_ERR_BADARGS;

    if (~(joypad->buttons & keys) & keys)
    {
        printf("JOYPAD\n");
        joypad->gbm->hwio->data[UGB_HWIO_REG_IF] |= UGB_REG_IE_X_MSK;
    }

    joypad->buttons |= keys;

    return UGB_ERR_OK;
}

int ugb_joypad_release(ugb_joypad* joypad, uint8_t keys)
{
    if (!joypad)
        return UGB_ERR_BADARGS;

    joypad->buttons &= ~keys;

    return UGB_ERR_OK;
}


int ugb_joypad_step(ugb_joypad* joypad)
{
    if (!joypad)
        return UGB_ERR_BADARGS;

    // HWIO register aliase
    uint8_t* hwreg_p1 = &joypad->gbm->hwio->data[UGB_HWIO_REG_P1];

    // Clear output lines
    *hwreg_p1 |= 0x0F;

    // Get current input line
    int in_line = ~((*hwreg_p1 & 0x30) >> 4);
    switch (in_line)
    {
        // Select from P14
        case 0x01:
            *hwreg_p1 &= ~(joypad->buttons & 0x0F);
            break;

        // Select from P15
        case 0x02:
            *hwreg_p1 &= ~((joypad->buttons >> 4) & 0x0F);
            break;
    }

    return UGB_ERR_OK;
}
