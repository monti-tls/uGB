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

#ifndef __GBM_JOYPAD_H__
#define __GBM_JOYPAD_H__

#include "gbm.h"

#include <stdint.h>

enum
{
    UGB_JOYPAD_RIGHT  = (0x01 << 0),
    UGB_JOYPAD_LEFT   = (0x01 << 1),
    UGB_JOYPAD_UP     = (0x01 << 2),
    UGB_JOYPAD_DOWN   = (0x01 << 3),

    UGB_JOYPAD_A      = (0x01 << 4),
    UGB_JOYPAD_B      = (0x01 << 5),
    UGB_JOYPAD_SELECT = (0x01 << 6),
    UGB_JOYPAD_START  = (0x01 << 7)
};

typedef struct ugb_joypad
{
    ugb_gbm* gbm;

    uint8_t buttons;
} ugb_joypad;

ugb_joypad* ugb_joypad_create(ugb_gbm* gbm);
void ugb_joypad_destroy(ugb_joypad* joypad);

int ugb_joypad_press(ugb_joypad* joypad, uint8_t keys);
int ugb_joypad_release(ugb_joypad* joypad, uint8_t keys);

int ugb_joypad_reset(ugb_joypad* joypad);
int ugb_joypad_step(ugb_joypad* joypad);

#endif // __GBM_JOYPAD_H__
