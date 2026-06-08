#include "slave.h"

static uint8_t _tab_bits[10] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1};

static int get_map_buf(int offset, int len, void *buf, int bufsz)
{
    uint8_t *ptr = (uint8_t *)buf;

    pthread_mutex_lock(&slave_mtx);
    for (int i = 0; i < len; i++) {
        ptr[i] = _tab_bits[offset + i];
    }
    pthread_mutex_unlock(&slave_mtx);

    return 0;
}

static int set_map_buf(int offset, int len, void *buf, int bufsz)
{
    uint8_t *ptr = (uint8_t *)buf;

    pthread_mutex_lock(&slave_mtx);
    for (int i = 0; i < len; i++) {
        _tab_bits[offset + i] = ptr[i];
    }
    pthread_mutex_unlock(&slave_mtx);

    return 0;
}

const agile_modbus_slave_util_map_t bit_maps[1] = {
    {0x041A, 0x0423, get_map_buf, set_map_buf}};
