#include "cpu.h"
#include "mmu.h"
#include "hwio.h"
#include "gpu.h"
#include "constants.h"
#include "errno.h"

#include "rom/bios.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

ugb_gbm* ugb_gbm_create()
{
    ugb_gbm* gbm = malloc(sizeof(ugb_gbm));
    if (!gbm)
        return 0;

    /*** Create hardware components ***/

    memset(gbm, 0, sizeof(ugb_gbm));
    if (!(gbm->cpu = ugb_cpu_create(gbm)) ||
        !(gbm->hwio = ugb_hwio_create(gbm)) ||
        !(gbm->gpu = ugb_gpu_create(gbm)) ||
        !(gbm->mmu = ugb_mmu_create(gbm)))
    {
        goto fail;
    }

    /*** Allocate internal GBM memory ***/

    if (!(gbm->mem.zpage = malloc(UGB_ZPAGE_SZ)) ||
        !(gbm->mem.ram0 = malloc(UGB_RAM0_SZ)))
        goto fail;

    /*** Create internal memory maps ***/

    // Map the BIOS first
    if (!(gbm->mem.bios_map = ugb_mmu_map_create(UGB_BIOS_LO, UGB_BIOS_HI)))
        goto fail;
    gbm->mem.bios_map->type = UGB_MMU_RODATA;
    gbm->mem.bios_map->rodata = &ugb_rom_bios[0];
    ugb_mmu_add_map(gbm->mmu, gbm->mem.bios_map);

    // Map the RAM0 bank
    ugb_mmu_map* ram0;
    if (!(ram0 = ugb_mmu_map_create(UGB_RAM0_LO, UGB_RAM0_HI)))
        goto fail;
    ram0->type = UGB_MMU_DATA;
    ram0->data = &gbm->mem.ram0[0];
    ugb_mmu_add_map(gbm->mmu, ram0);

    // Map the GPU's video RAM (character ram + BG maps 1 and 2)
    ugb_mmu_map* vram;
    if (!(vram = ugb_mmu_map_create(UGB_VRAM_LO, UGB_VRAM_HI)))
        goto fail;
    vram->type = UGB_MMU_DATA;
    vram->data = gbm->gpu->vram;
    ugb_mmu_add_map(gbm->mmu, vram);

    // Map the GPU's Object Attribute Memory
    ugb_mmu_map* oam;
    if (!(oam = ugb_mmu_map_create(UGB_OAM_LO, UGB_OAM_HI)))
        goto fail;
    oam->type = UGB_MMU_DATA;
    oam->data = gbm->gpu->oam;
    ugb_mmu_add_map(gbm->mmu, oam);

    // Map hardware IO registers
    ugb_mmu_map* hwio;
    if (!(hwio = ugb_mmu_map_create(UGB_HWIO_LO, UGB_HWIO_HI)))
        goto fail;
    hwio->type = UGB_MMU_SOFT;
    hwio->soft.handler = &ugb_hwio_mmu_handler;
    hwio->soft.cookie = (void*) gbm->hwio;
    ugb_mmu_add_map(gbm->mmu, hwio);

    // Map the Zero Page
    ugb_mmu_map* zpage;
    if (!(zpage = ugb_mmu_map_create(UGB_ZPAGE_LO, UGB_ZPAGE_HI)))
        goto fail;
    zpage->type = UGB_MMU_DATA;
    zpage->data = &gbm->mem.zpage[0];
    ugb_mmu_add_map(gbm->mmu, zpage);

    // Map the CPU's IE register using MMU soft interface
    ugb_mmu_map* iereg;
    if (!(iereg = ugb_mmu_map_create(UGB_IEREG_LO, UGB_IEREG_HI)))
        goto fail;
    iereg->type = UGB_MMU_SOFT;
    iereg->soft.handler = &ugb_cpu_iereg_mmu_handler;
    iereg->soft.cookie = gbm->cpu;
    ugb_mmu_add_map(gbm->mmu, iereg);

    /*** Last configs... */

    if (ugb_hwio_set_hook(gbm->hwio, UGB_HWIO_REG_BD, &ugb_gbm_bdreg_hook, (void*) gbm) != UGB_ERR_OK)
        goto fail;

    /*** Reset the GBM system ***/

    if (ugb_gbm_reset(gbm) != UGB_ERR_OK)
        goto fail;

    return gbm;

fail:
    ugb_gbm_destroy(gbm);
    return 0;
}

void ugb_gbm_destroy(ugb_gbm* gbm)
{
    if (gbm)
    {
        free(gbm->mem.zpage);
        free(gbm->mem.ram0);

        ugb_mmu_destroy(gbm->mmu);
        ugb_gpu_destroy(gbm->gpu);
        ugb_hwio_destroy(gbm->hwio);
        ugb_cpu_destroy(gbm->cpu);

        free(gbm);
    }
}

int ugb_gbm_step(ugb_gbm* gbm)
{
    if (!gbm)
        return UGB_ERR_BADARGS;

    int err;
    size_t cycles;

    if ((err = ugb_cpu_step(gbm->cpu, &cycles)) != UGB_ERR_OK ||
        (err = ugb_gpu_step(gbm->gpu, cycles)) != UGB_ERR_OK)
        return err;

    return UGB_ERR_OK;
}

int ugb_gbm_reset(ugb_gbm* gbm)
{
    if (!gbm)
        return UGB_ERR_BADARGS;

    // Enable the BIOS ROM
    gbm->mem.bios_map->type = UGB_MMU_RODATA;

    // Reset hardware components
    int err;
    if ((err = ugb_cpu_reset(gbm->cpu)) != UGB_ERR_OK ||
        (err = ugb_gpu_reset(gbm->gpu)) != UGB_ERR_OK ||
        (err = ugb_hwio_reset(gbm->hwio)) != UGB_ERR_OK)
        return err;

    return UGB_ERR_OK;
}

int ugb_gbm_bdreg_hook(struct ugb_hwreg* reg, void* cookie)
{
    if (!reg || !cookie)
        return UGB_ERR_BADARGS;

    ugb_gbm* gbm = (ugb_gbm*) cookie;

    // Remove the BIOS ROM mapping so the 0000 -> 00FF zone is
    //   accessible on the cartridge's ROM0
    gbm->mem.bios_map->type = UGB_MMU_NONE;

    return UGB_ERR_OK;
}
