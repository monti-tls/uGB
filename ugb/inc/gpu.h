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

#ifndef __GBM_GPU_H__
#define __GBM_GPU_H__

#include "gbm.h"

#include <stdint.h>

typedef struct ugb_gpu
{
    ugb_gbm* gbm;

    size_t clock;
    size_t mode_clocks[4];

    uint8_t* framebuf;
    uint8_t* vram;
    uint8_t* oam;
} ugb_gpu;

ugb_gpu* ugb_gpu_create(ugb_gbm* gbm);
void ugb_gpu_destroy(ugb_gpu* gpu);

int ugb_gpu_reset(ugb_gpu* gpu);
int ugb_gpu_step(ugb_gpu* gpu, size_t cycles);

#endif // __GBM_GPU_H__
