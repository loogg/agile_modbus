#include "slave.h"

extern uint16_t tab_input_registers[1000];

static int get_map_buf(const agile_modbus_slave_util_map_t *map, void *buf, int bufsz)
{
    uint16_t *ptr = (uint16_t *)buf;

    pthread_mutex_lock(&slave_mtx);
    memcpy(ptr, tab_input_registers + map->addr_offset, map->addr_len * 2);
    pthread_mutex_unlock(&slave_mtx);

    return 0;
}

const agile_modbus_slave_util_map_t input_register_maps[1] = {
    {0x0000, 0x03E7, get_map_buf, NULL}};
