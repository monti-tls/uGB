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

#ifndef __GBM_TIMER_H__
#define __GBM_TIMER_H__

#include "gbm.h"

#include <stdint.h>

typedef struct ugb_timer
{
    ugb_gbm* gbm;

    size_t clock0;
    size_t clock1;
} ugb_timer;

ugb_timer* ugb_timer_create(ugb_gbm* gbm);
void ugb_timer_destroy(ugb_timer* timer);

int ugb_timer_reset(ugb_timer* timer);
int ugb_timer_step(ugb_timer* timer, size_t cycles);

int ugb_timer_div_hook(struct ugb_hwreg* reg, void* cookie);

#endif // __GBM_TIMER_H__
