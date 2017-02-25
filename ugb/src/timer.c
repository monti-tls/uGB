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

    timer->clock = 0;

    return UGB_ERR_OK;
}

int ugb_timer_step(ugb_timer* timer, size_t cycles)
{
    if (!timer)
        return UGB_ERR_BADARGS;

    // HWIO register aliases
    uint8_t tac = timer->gbm->hwio->data[UGB_HWIO_REG_TAC];
    uint8_t* tima = &timer->gbm->hwio->data[UGB_HWIO_REG_TIMA];
    uint8_t tma = timer->gbm->hwio->data[UGB_HWIO_REG_TMA];

    // If enabled...
    if (tac & (0x01 << 2))
    {
        int div = tac & 0x3;

        static const size_t div_map[] =
        {
            UGB_TIMER_DIV00,
            UGB_TIMER_DIV01,
            UGB_TIMER_DIV10,
            UGB_TIMER_DIV11,
        };

        // Divide input clock
        timer->clock += cycles;
        while (timer->clock >= div_map[div])
        {
            timer->clock -= div_map[div];

            // On overflow
            if (*tima == 0xFF)
            {
                // Reset counter to start value
                *tima = tma;
                // Raise interrupt flag in CPU
                timer->gbm->hwio->data[UGB_HWIO_REG_IF] |= (0x01 << UGB_REG_IE_T_MSK);
            }
            else
                ++(*tima);
        }
    }

    return UGB_ERR_OK;
}
