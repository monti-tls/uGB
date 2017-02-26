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

#ifndef __UGB_MMU_H__
#define __UGB_MMU_H__

#include <stdint.h>

#include "gbm.h"

enum
{
    UGB_MMU_NONE,
    UGB_MMU_DATA,
    UGB_MMU_RODATA,
    UGB_MMU_SOFT
};

enum
{
    UGB_MMU_READ,
    UGB_MMU_WRITE
};

typedef struct ugb_mmu_map
{
    uint16_t low_addr;
    uint16_t high_addr;

    int type;
    union
    {
        void* target_ptr;
        uint8_t* data;
        uint8_t const* rodata;
        struct
        {
            int (*handler)(void*, int, uint16_t, uint8_t*);
            void* cookie;
        } soft;
    };

    struct ugb_mmu_map* prev;
    struct ugb_mmu_map* next;
} ugb_mmu_map;

typedef struct ugb_mmu
{
    ugb_gbm* gbm;

    ugb_mmu_map* maps;
    ugb_mmu_map* last_map;
} ugb_mmu;

ugb_mmu* ugb_mmu_create(ugb_gbm* gbm);
void ugb_mmu_destroy(ugb_mmu* mmu);

ugb_mmu_map* ugb_mmu_map_create(uint16_t low_addr, uint16_t high_addr);

int ugb_mmu_add_map(ugb_mmu* mmu, ugb_mmu_map* map);
int ugb_mmu_remove_map(ugb_mmu* mmu, ugb_mmu_map* map);
int ugb_mmu_clear_maps(ugb_mmu* mmu);
ugb_mmu_map* ugb_mmu_resolve_map(ugb_mmu* mmu, uint16_t addr);

int ugb_mmu_read(ugb_mmu* mmu, uint16_t addr, uint8_t* data);
int ugb_mmu_write(ugb_mmu* mmu, uint16_t addr, uint8_t data);

#endif // __UGB_MMU_H__
