#ifndef __RT_TICK_H
#define __RT_TICK_H
#include <stdint.h>

#define RT_TICK_MAX        0xffffffff
#define RT_TICK_PER_SECOND 200
#define RT_WAITING_FOREVER -1

void rt_tick_init(void);
uint32_t rt_tick_get(void);
uint32_t rt_tick_from_millisecond(int32_t ms);

#endif
