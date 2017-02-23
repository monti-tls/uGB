#ifndef __UGB_HWIO_H__
#define __UGB_HWIO_H__

#include "gbm.h"

enum
{
    #define DEF_HWREG(offset, name, ...) UGB_HWIO_REG_ ## name = 0x ## offset,
    #include "hwio.def"

    UGB_HWIO_REG_SIZE
};

typedef struct ugb_hwreg
{
    const char* name;
    uint8_t wmask;
    uint8_t rmask;
    uint8_t umask;
    uint8_t reset;
} ugb_hwreg;

typedef struct ugb_hwio
{
    ugb_gbm* gbm;

    ugb_hwreg regs[UGB_HWIO_REG_SIZE];
    uint8_t data[UGB_HWIO_REG_SIZE];
} ugb_hwio;

ugb_hwio* ugb_hwio_create(ugb_gbm* gbm);
void ugb_hwio_destroy(ugb_hwio* hwio);

int ugb_hwio_reset(ugb_hwio* hwio);
int ugb_hwio_mmu_handler(void* cookie, int op, uint16_t offset, uint8_t* data);

#endif // __UGB_HWIO_H__
