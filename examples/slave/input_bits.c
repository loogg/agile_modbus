#include "slave.h"

static uint8_t _tab_input_bits[10] = {0, 1, 1, 0, 0, 1, 1, 0, 0, 1};

static int get_map_buf(int index, void *buf, int bufcnt)
{
    uint8_t *ptr = (uint8_t *)buf;

    pthread_mutex_lock(&slave_mtx);
    for (int i = 0; i < bufcnt && i + index < sizeof(_tab_input_bits); i++) {
        ptr[i] = _tab_input_bits[index + i];
    }
    pthread_mutex_unlock(&slave_mtx);

    return 0;
}

const agile_modbus_slave_util_map_t input_bit_maps[1] = {
    {0x041A, 0x0423, get_map_buf, NULL}};
