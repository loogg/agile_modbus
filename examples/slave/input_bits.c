#include "slave.h"

extern uint8_t tab_input_bits[1000];

static int get_map_buf(const agile_modbus_slave_util_map_t *map, void *buf, int bufsz)
{
    uint8_t *ptr = (uint8_t *)buf;

    pthread_mutex_lock(&slave_mtx);
    memcpy(ptr, tab_input_bits + map->addr_offset, map->addr_len);
    pthread_mutex_unlock(&slave_mtx);

    return 0;
}

const agile_modbus_slave_util_map_t input_bit_maps[1] = {
    {0x0000, 0x03E7, get_map_buf, NULL}};
