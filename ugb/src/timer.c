#include "timer.h"
#include "gbm.h"
#include "hwio.h"
#include "cpu.h"
#include "constants.h"
#include "errno.h"

#include <stdlib.h>
#include <string.h>

ugb_timer* ugb_timer_create(ugb_gbm* gbm)
{
    ugb_timer* timer = malloc(sizeof(ugb_timer));
    if (!timer)
        return 0;

    memset(timer, 0, sizeof(ugb_timer));
    timer->gbm = gbm;

    if (ugb_hwio_set_hook(gbm->hwio, UGB_HWIO_REG_DIV, &ugb_timer_div_hook, (void*) gbm) != UGB_ERR_OK)
    {
        ugb_timer_destroy(timer);
        return 0;
    }

    return timer;
}

void ugb_timer_destroy(ugb_timer* timer)
{
    if (timer)
    {
        free(timer);
    }
}

int ugb_timer_reset(ugb_timer* timer)
{
    if (!timer)
        return UGB_ERR_BADARGS;

    timer->clock0 = 0;
    timer->clock1 = 0;

    return UGB_ERR_OK;
}

int ugb_timer_step(ugb_timer* timer, size_t cycles)
{
    if (!timer)
        return UGB_ERR_BADARGS;

    // HWIO register aliases
    uint8_t tac = timer->gbm->hwio->data[UGB_HWIO_REG_TAC];
    uint8_t* tima = &timer->gbm->hwio->data[UGB_HWIO_REG_TIMA];
    uint8_t* div = &timer->gbm->hwio->data[UGB_HWIO_REG_DIV];
    uint8_t tma = timer->gbm->hwio->data[UGB_HWIO_REG_TMA];

    // If enabled...
    if (tac & (0x01 << 2))
    {
        // Manage the DIV register which has fixed speed
        timer->clock0 += cycles;
        while (timer->clock0 >= UGB_TIMER_DIV)
        {
            timer->clock0 -= UGB_TIMER_DIV;
            ++(*div);
        }

        // Get the clock scaling with the timer configuration
        int scale = tac & 0x3;
        static const size_t div_map[] =
        {
            UGB_TIMER_DIV00,
            UGB_TIMER_DIV01,
            UGB_TIMER_DIV10,
            UGB_TIMER_DIV11,
        };

        // Manage the TIMA register which has configurable speed
        timer->clock1 += cycles;
        while (timer->clock1 >= div_map[scale])
        {
            timer->clock1 -= div_map[scale];

            // On overflow
            if (*tima == 0xFF)
            {
                // Reset counter to start value
                *tima = tma;

                // Raise interrupt flag in CPU
                timer->gbm->hwio->data[UGB_HWIO_REG_IF] |= UGB_REG_IE_T_MSK;
            }
            else
                ++(*tima);
        }
    }

    return UGB_ERR_OK;
}

int ugb_timer_div_hook(struct ugb_hwreg* reg, void* cookie)
{
    if (!reg || !cookie)
        return UGB_ERR_BADARGS;

    ugb_gbm* gbm = (ugb_gbm*) cookie;

    // Any write to the DIV register writes a 0
    gbm->hwio->data[UGB_HWIO_REG_DIV] = 0;
    gbm->hwio->data[UGB_HWIO_REG_TIMA] = gbm->hwio->data[UGB_HWIO_REG_TMA];

    return UGB_ERR_OK;
}
