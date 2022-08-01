#include "slave.h"
#include <stdio.h>
#include <stdlib.h>
#include "rt_tick.h"

#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME "slave"
#define DBG_LEVEL        DBG_LOG
#include "dbg_log.h"

extern const agile_modbus_slave_util_map_t bit_maps[1];
extern const agile_modbus_slave_util_map_t input_bit_maps[1];
extern const agile_modbus_slave_util_map_t register_maps[1];
extern const agile_modbus_slave_util_map_t input_register_maps[1];

extern int rtu_slave_init(const char *dev, pthread_t *tid);
extern int tcp_slave_init(int port, pthread_t *tid);

pthread_mutex_t slave_mtx;

static int addr_check(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info)
{
    int slave = slave_info->sft->slave;
    if ((slave != ctx->slave) && (slave != AGILE_MODBUS_BROADCAST_ADDRESS) && (slave != 0xFF))
        return -AGILE_MODBUS_EXCEPTION_UNKNOW;

    return 0;
}

const agile_modbus_slave_util_t slave_util = {
    bit_maps,
    sizeof(bit_maps) / sizeof(bit_maps[0]),
    input_bit_maps,
    sizeof(input_bit_maps) / sizeof(input_bit_maps[0]),
    register_maps,
    sizeof(register_maps) / sizeof(register_maps[0]),
    input_register_maps,
    sizeof(input_register_maps) / sizeof(input_register_maps[0]),
    addr_check,
    NULL,
    NULL};

int main(int argc, char *argv[])
{
    if (argc < 3) {
        LOG_E("Please enter ModbusSlave [dev] [port]!");
        return -1;
    }

    pthread_mutex_init(&slave_mtx, NULL);

    rt_tick_init();

    pthread_t rtu_tid;
    pthread_t tcp_tid;

    int rc1 = rtu_slave_init(argv[1], &rtu_tid);
    int rc2 = tcp_slave_init(atoi(argv[2]), &tcp_tid);

    if (rc1 == 0)
        pthread_join(rtu_tid, NULL);

    if (rc2 == 0)
        pthread_join(tcp_tid, NULL);

    return 0;
}
