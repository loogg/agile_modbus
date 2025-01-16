#include "slave.h"

extern uint16_t tab_registers[1000];

static int get_map_buf(const agile_modbus_slave_util_map_t *map, void *buf, int bufsz)
{
    uint16_t *ptr = (uint16_t *)buf;

    pthread_mutex_lock(&slave_mtx);
    memcpy(ptr, tab_registers + map->addr_offset, map->addr_len * 2);
    pthread_mutex_unlock(&slave_mtx);

    return 0;
}

static int set_map_buf(const agile_modbus_slave_util_map_t *map, int index, int len, void *buf, int bufsz)
{
    uint16_t *ptr = (uint16_t *)buf;

    pthread_mutex_lock(&slave_mtx);
    for (int i = 0; i < len; i++) {
        tab_registers[map->addr_offset + index + i] = ptr[index + i];
    }
    pthread_mutex_unlock(&slave_mtx);

    return 0;
}

const agile_modbus_slave_util_map_t register_maps[1] = {
    {0x0000, 0x03E7, get_map_buf, set_map_buf}};
