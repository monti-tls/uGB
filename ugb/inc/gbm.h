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

typedef struct ugb_gbm
{
    struct ugb_cpu* cpu;
    struct ugb_hwio* hwio;
    struct ugb_gpu* gpu;

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

int ugb_gbm_step(ugb_gbm* gbm);
int ugb_gbm_reset(ugb_gbm* gbm);

int ugb_gbm_bdreg_hook(struct ugb_hwreg* reg, void* cookie);

#endif // __UGB_GBM_H__
