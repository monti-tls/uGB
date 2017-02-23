#include "gpu.h"
#include "hwio.h"
#include "constants.h"
#include "errno.h"

#include <stdlib.h>
#include <string.h>

ugb_gpu* ugb_gpu_create(ugb_gbm* gbm)
{
    ugb_gpu* gpu = malloc(sizeof(ugb_gpu));
    if (!gpu)
        return 0;

    memset(gpu, 0, sizeof(ugb_gpu));
    gpu->gbm = gbm;

    gpu->mode_clocks[0] = GBM_GPU_MODE00_CLOCKS;
    gpu->mode_clocks[1] = GBM_GPU_MODE01_CLOCKS;
    gpu->mode_clocks[2] = GBM_GPU_MODE10_CLOCKS;
    gpu->mode_clocks[3] = GBM_GPU_MODE11_CLOCKS;

    return gpu;
}

void ugb_gpu_destroy(ugb_gpu* gpu)
{
    if (gpu)
    {
        free(gpu);
    }
}

int ugb_gpu_reset(ugb_gpu* gpu)
{
    if (!gpu)
        return UGB_ERR_BADARGS;

    //TODO: reset buffers and everything
    gpu->clock = 0;

    return UGB_ERR_OK;
}

int ugb_gpu_step(ugb_gpu* gpu, size_t cycles)
{
    if (!gpu)
        return UGB_ERR_BADARGS;

    // Aliases to relevant HWIO registers
    uint8_t* hwreg_stat = &gpu->gbm->hwio->data[UGB_HWIO_REG_STAT];

    // Get current mode
    int mode = *hwreg_stat & 0x3;

    // Increment GPU clock, see if we need to do something
    gpu->clock += cycles;
    while (gpu->clock >= gpu->mode_clocks[mode])
    {
        gpu->clock -= gpu->mode_clocks[mode];

        // Emulate GPU hardware
        switch (mode)
        {
            case 0: // HBlank
            {
                break;
            }

            case 1: // VBlank
            {
                break;
            }

            case 2: // OAM
            {
                break;
            }

            case 3: // VRAM
            {
                break;
            }
        }
    }

    // Update HWIO registers
    *hwreg_stat = (*hwreg_stat & ~0x3) | mode;

    return UGB_ERR_OK;
}
