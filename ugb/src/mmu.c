#include "mmu.h"
#include "errno.h"

#include <stdlib.h>
#include <string.h>

ugb_mmu* ugb_mmu_create(ugb_gbm* gbm)
{
    if (!gbm)
        return 0;

    ugb_mmu* mmu = malloc(sizeof(ugb_mmu));
    if (!mmu)
        return 0;

    memset(mmu, 0, sizeof(ugb_mmu));
    mmu->gbm = gbm;

    return mmu;
}

void ugb_mmu_destroy(ugb_mmu* mmu)
{
    if (mmu)
    {
        for (ugb_mmu_map* map = mmu->maps; map; )
        {
            ugb_mmu_map* next = map->next;
            free(map);
            map = next;
        }

        free(mmu);
    }
}

int ugb_mmu_add_map(ugb_mmu* mmu, ugb_mmu_map* map)
{
    if (!mmu || !map || !map->target_ptr)
        return UGB_ERR_BADARGS;

    for (ugb_mmu_map* other = mmu->maps; other; other = other->next)
    {
        if (map->low_addr >= other->low_addr && map->high_addr <= other->high_addr)
            return UGB_ERR_MMU_CLASH;
    }

    map->prev = mmu->last_map;
    map->next = 0;

    if (map->prev)
        map->prev->next = map;
    else
        mmu->maps = map;

    mmu->last_map = map;

    return UGB_ERR_OK;
}

int ugb_mmu_remove_map(ugb_mmu* mmu, ugb_mmu_map* map)
{
    if (!mmu || !map)
        return UGB_ERR_BADARGS;

    if (map->prev)
        map->prev->next = map->next;
    else
        mmu->maps = map->next;

    if (map->next)
        map->next->prev = map->prev;
    else
        mmu->last_map = map->prev;

    return UGB_ERR_OK;
}

ugb_mmu_map* ugb_mmu_resolve_map(ugb_mmu* mmu, uint16_t addr)
{
    if (!mmu)
        return 0;

    for (ugb_mmu_map* map = mmu->maps; map; map = map->next)
        if (addr >= map->low_addr && addr <= map->high_addr)
            return map;

    return 0;
}

int ugb_mmu_read(ugb_mmu* mmu, uint16_t addr, uint8_t* data)
{
    if (!mmu || !data)
        return UGB_ERR_BADARGS;

    ugb_mmu_map* map = ugb_mmu_resolve_map(mmu, addr);
    if (!map)
        return UGB_ERR_MMU_MAP;

    switch (map->type)
    {
        case UGB_MMU_DATA:
            *data = map->data[addr - map->low_addr];
            break;

        case UGB_MMU_RODATA:
            *data = map->rodata[addr - map->low_addr];
            break;

        case UGB_MMU_SOFT:
            return (*map->soft_handler)(map, UGB_MMU_READ, addr, data);

        default:
            return UGB_ERR_BADCONF;
    }

    return UGB_ERR_OK;
}

int ugb_mmu_write(ugb_mmu* mmu, uint16_t addr, uint8_t data)
{
    if (!mmu || !data)
        return UGB_ERR_BADARGS;

    ugb_mmu_map* map = ugb_mmu_resolve_map(mmu, addr);
    if (!map)
        return UGB_ERR_MMU_MAP;

    switch (map->type)
    {
        case UGB_MMU_DATA:
            map->data[addr - map->low_addr] = data;
            break;

        case UGB_MMU_RODATA:
            return UGB_ERR_MMU_RO;

        case UGB_MMU_SOFT:
            return (*map->soft_handler)(map, UGB_MMU_WRITE, addr, &data);

        default:
            return UGB_ERR_BADCONF;
    }

    return UGB_ERR_OK;
}
