#ifndef __UGB_HWREGS_H__
#define __UGB_HWREGS_H__

#include "gbm.h"

enum
{
    #define DEF_HWREG(offset, name, ...) UGB_HWREG_ ## name = (0x ## offset),
    #include "hwregs.def"

    UGB_HWREG_SIZE
};

typedef struct ugb_hwreg
{
    uint8_t wmask;
    uint8_t rmask;
    uint8_t umask;
    uint8_t reset;
} ugb_hwreg;

typedef struct ugb_hwio
{
    ugb_hwreg regs[UGB_HWREG_SIZE];
    uint8_t data[UGB_HWREG_SIZE];
} ugb_hwio;

ugb_hwio* ugb_hwio_create(ugb_gbm* gbm);
void ugb_hwio_destroy(ugb_hwio* hwio);

#endif // __UGB_HWREGS_H__
