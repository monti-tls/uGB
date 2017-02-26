#ifndef __GBM_TIMER_H__
#define __GBM_TIMER_H__

#include "gbm.h"

#include <stdint.h>

typedef struct ugb_timer
{
    ugb_gbm* gbm;

    size_t clock0;
    size_t clock1;
} ugb_timer;

ugb_timer* ugb_timer_create(ugb_gbm* gbm);
void ugb_timer_destroy(ugb_timer* timer);

int ugb_timer_reset(ugb_timer* timer);
int ugb_timer_step(ugb_timer* timer, size_t cycles);

int ugb_timer_div_hook(struct ugb_hwreg* reg, void* cookie);

#endif // __GBM_TIMER_H__
