#include "cpu.h"
#include "mmu.h"
#include "hwio.h"
#include "gpu.h"

#include <stdlib.h>
#include <string.h>

ugb_gbm* ugb_gbm_create()
{
    ugb_gbm* gbm = malloc(sizeof(ugb_gbm));
    if (!gbm)
        return 0;

    memset(gbm, 0, sizeof(ugb_gbm));
    if (!(gbm->cpu = ugb_cpu_create(gbm)) ||
        !(gbm->mmu = ugb_mmu_create(gbm)) ||
        !(gbm->hwio = ugb_hwio_create(gbm)) ||
        !(gbm->gpu = ugb_gpu_create(gbm)))
    {
        ugb_gbm_destroy(gbm);
        return 0;
    }

    return gbm;
}

void ugb_gbm_destroy(ugb_gbm* gbm)
{
    if (gbm)
    {
        ugb_gpu_destroy(gbm->gpu);
        ugb_hwio_destroy(gbm->hwio);
        ugb_mmu_destroy(gbm->mmu);
        ugb_cpu_destroy(gbm->cpu);
        free(gbm);
    }
}
