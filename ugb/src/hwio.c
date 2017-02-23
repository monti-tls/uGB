#include "hwio.h"
#include "mmu.h"
#include "errno.h"

#include <stdlib.h>
#include <string.h>

ugb_hwio* ugb_hwio_create(ugb_gbm* gbm)
{
    ugb_hwio* hwio = malloc(sizeof(ugb_hwio));
    if (!hwio)
        return 0;

    memset(hwio, 0, sizeof(ugb_hwio));
    hwio->gbm = 0;

    #define DEF_HWREG(offset, name_, reset_, wmask_, rmask_, umask_) \
    hwio->regs[0x ## offset].name = #name_; \
    hwio->regs[0x ## offset].wmask = wmask_; \
    hwio->regs[0x ## offset].rmask = rmask_; \
    hwio->regs[0x ## offset].umask = umask_; \
    hwio->regs[0x ## offset].reset = reset_;
    #include "hwio.def"

    ugb_hwio_reset(hwio);

    return hwio;
}

void ugb_hwio_destroy(ugb_hwio* hwio)
{
    if (hwio)
        free(hwio);
}

int ugb_hwio_reset(ugb_hwio* hwio)
{
    if (!hwio)
        return UGB_ERR_BADARGS;

    for (int i = 0; i < UGB_HWIO_REG_SIZE; ++i)
        hwio->data[i] = hwio->regs[i].reset;

    return 0;
}

int ugb_hwio_mmu_handler(void* cookie, int op, uint16_t offset, uint8_t* data)
{
    if (!cookie)
        return UGB_ERR_BADARGS;

    ugb_hwio* hwio = (ugb_hwio*) cookie;
    ugb_hwreg* reg = offset < UGB_HWIO_REG_SIZE && hwio->regs[offset].name ? &hwio->regs[offset] : 0;

    if (!reg)
        return UGB_ERR_OK;

    switch (op)
    {
        case UGB_MMU_READ:
        {
            *data = (hwio->data[offset] & reg->rmask) & (~reg->umask);
            break;
        }

        case UGB_MMU_WRITE:
        {
            //TODO: handle writes
            break;
        }
    }

    return UGB_ERR_OK;
}
