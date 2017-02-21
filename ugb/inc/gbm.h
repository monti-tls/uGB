#ifndef __UGB_GBM_H__
#define __UGB_GBM_H__

#include <stdint.h>
#include <unistd.h>

struct ugb_cpu;
struct ugb_mmu;

typedef struct ugb_gbm
{
    struct ugb_cpu* cpu;
    struct ugb_mmu* mmu;
} ugb_gbm;

ugb_gbm* ugb_gbm_create();
void ugb_gbm_destroy(ugb_gbm* gbm);

#endif // __UGB_GBM_H__
