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
} ugb_gpu;

ugb_gpu* ugb_gpu_create(ugb_gbm* gbm);
void ugb_gpu_destroy(ugb_gpu* gpu);

int ugb_gpu_reset(ugb_gpu* gpu);
int ugb_gpu_step(ugb_gpu* gpu, size_t cycles);

#endif // __GBM_GPU_H__
