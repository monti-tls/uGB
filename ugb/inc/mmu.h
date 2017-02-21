#ifndef __UGB_MMU_H__
#define __UGB_MMU_H__

#include <stdint.h>

#include "gbm.h"

enum
{
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
        int (*soft_handler)(struct ugb_mmu_map*, int, uint16_t, uint8_t*);
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

int ugb_mmu_add_map(ugb_mmu* mmu, ugb_mmu_map* map);
int ugb_mmu_remove_map(ugb_mmu* mmu, ugb_mmu_map* map);
ugb_mmu_map* ugb_mmu_resolve_map(ugb_mmu* mmu, uint16_t addr);

int ugb_mmu_read(ugb_mmu* mmu, uint16_t addr, uint8_t* data);
int ugb_mmu_write(ugb_mmu* mmu, uint16_t addr, uint8_t data);

#endif // __UGB_MMU_H__
