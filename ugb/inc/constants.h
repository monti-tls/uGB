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

#ifndef __GBM_CONSTANTS_H__
#define __GBM_CONSTANTS_H__

#include <stdint.h>

#define UGB_CPU_CLOCK_FREQ    4194304.0L

#define UGB_GPU_MODE00_CLOCKS (4*204)
#define UGB_GPU_MODE01_CLOCKS (4*450)
#define UGB_GPU_MODE10_CLOCKS (4*80)
#define UGB_GPU_MODE11_CLOCKS (4*172)

#define UGB_GPU_SCREEN_W      160
#define UGB_GPU_SCREEN_H      144
#define UGB_GPU_SCREEN_VH     154

#define UGB_TIMER_DIV         256  // 16
#define UGB_TIMER_DIV00       1024 // 64
#define UGB_TIMER_DIV01       16   // 1
#define UGB_TIMER_DIV10       64   // 4
#define UGB_TIMER_DIV11       256  // 16

#define UGB_BIOS_LO      0x0000
#define UGB_BIOS_HI      0x00FF
#define UGB_BIOS_SZ      0x0100

#define UGB_CART_ROM0_LO 0x0000
#define UGB_CART_ROM0_HI 0x3FFF
#define UGB_CART_ROM0_SZ 0x4000

#define UGB_RAM0_LO      0xC000
#define UGB_RAM0_HI      0xDFFF
#define UGB_RAM0_SZ      0x2000

#define UGB_OAM_LO       0xFE00
#define UGB_OAM_HI       0xFE9F
#define UGB_OAM_SZ       0x00A0

#define UGB_HWIO_LO      0xFF00
#define UGB_HWIO_HI      0xFF7F
#define UGB_HWIO_SZ      0x0080

#define UGB_VRAM_LO      0x8000
#define UGB_VRAM_HI      0x9FFF
#define UGB_VRAM_SZ      0x2000

#define UGB_ZPAGE_LO     0xFF80
#define UGB_ZPAGE_HI     0xFFFE
#define UGB_ZPAGE_SZ     0x007F

#define UGB_IEREG_LO     0xFFFF
#define UGB_IEREG_HI     0xFFFF
#define UGB_IEREG_SZ     0x0001

#endif // __GBM_CONSTANTS_H__
