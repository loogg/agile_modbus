#include "slave.h"

static uint16_t _tab_registers[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

static int get_map_buf(int index, void *buf, int bufcnt)
{
    uint16_t *ptr = (uint16_t *)buf;

    pthread_mutex_lock(&slave_mtx);
    for (int i = 0; i < bufcnt && i + index < sizeof(_tab_registers) / sizeof(_tab_registers[0]); i++) {
        ptr[i] = _tab_registers[index + i];
    }
    pthread_mutex_unlock(&slave_mtx);

    return 0;
}

static int set_map_buf(int index, void *buf, int bufcnt)
{
    uint16_t *ptr = (uint16_t *)buf;

    pthread_mutex_lock(&slave_mtx);
    for (int i = 0; i < bufcnt && i + index < sizeof(_tab_registers) / sizeof(_tab_registers[0]); i++) {
        _tab_registers[index + i] = ptr[i];
    }
    pthread_mutex_unlock(&slave_mtx);

    return 0;
}

const agile_modbus_slave_util_map_t register_maps[1] = {
    {0xFFF6, 0xFFFF, get_map_buf, set_map_buf}};
