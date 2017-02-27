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

#ifndef __UGB_GBM_H__
#define __UGB_GBM_H__

#include <stdint.h>
#include <unistd.h>

struct ugb_cpu;
struct ugb_mmu;
struct ugb_mmu_map;
struct ugb_hwio;
struct ugb_hwreg;
struct ugb_gpu;
struct ugb_timer;
struct ugb_joypad;

typedef struct ugb_gbm
{
    struct ugb_cpu* cpu;
    struct ugb_hwio* hwio;
    struct ugb_gpu* gpu;
    struct ugb_timer* timer;
    struct ugb_joypad* joypad;

    struct ugb_mmu* mmu;

    struct
    {
        struct ugb_mmu_map* bios_map;
        uint8_t* ram0;
        uint8_t* zpage;
    } mem;
} ugb_gbm;

ugb_gbm* ugb_gbm_create();
void ugb_gbm_destroy(ugb_gbm* gbm);

int ugb_gbm_reset(ugb_gbm* gbm);
int ugb_gbm_step(ugb_gbm* gbm, double* us);

int ugb_gbm_bdreg_hook(struct ugb_hwreg* reg, void* cookie);

#endif // __UGB_GBM_H__
