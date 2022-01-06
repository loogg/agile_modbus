#include "rt_tick.h"
#include <pthread.h>
#include <unistd.h>

static uint32_t rt_tick = 0;
static uint8_t is_initialized = 0;

static void *rt_tick_entry(void *param)
{
    int us = 1000000 / RT_TICK_PER_SECOND;
    while (1) {
        usleep(us);
        rt_tick++;
    }

    pthread_exit(NULL);
}

void rt_tick_init(void)
{
    if (is_initialized)
        return;

    pthread_t tid;
    pthread_create(&tid, NULL, rt_tick_entry, NULL);
    pthread_detach(tid);

    is_initialized = 1;
}

uint32_t rt_tick_get(void)
{
    return rt_tick;
}

uint32_t rt_tick_from_millisecond(int32_t ms)
{
    uint32_t tick;

    if (ms < 0) {
        tick = (uint32_t)RT_WAITING_FOREVER;
    } else {
        tick = RT_TICK_PER_SECOND * (ms / 1000);
        tick += (RT_TICK_PER_SECOND * (ms % 1000) + 999) / 1000;
    }

    /* return the calculated tick */
    return tick;
}
