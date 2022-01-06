#include "slave.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "rt_tick.h"

#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME "slave"
#define DBG_LEVEL        DBG_LOG
#include "dbg_log.h"

#define TAB_MAX_NUM 10

static pthread_mutex_t _mtx;
static uint8_t _tab_bits[TAB_MAX_NUM] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
static uint8_t _tab_input_bits[TAB_MAX_NUM] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
static uint16_t _tab_registers[TAB_MAX_NUM] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
static uint16_t _tab_input_registers[TAB_MAX_NUM] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

/**
 * @brief   从机回调函数
 * @param   ctx modbus 句柄
 * @param   slave_info 从机信息体
 * @return  =0:正常;
 *          <0:异常
 *             (-AGILE_MODBUS_EXCEPTION_UNKNOW(-255): 未知异常，从机不会打包响应数据)
 *             (其他负数异常码: 从机会打包异常响应数据)
 */
int slave_callback(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info)
{
    int function = slave_info->sft->function;

    switch (function) {
    case AGILE_MODBUS_FC_READ_COILS:
    case AGILE_MODBUS_FC_READ_DISCRETE_INPUTS: {
        int address = slave_info->address;
        int nb = slave_info->nb;
        int send_index = slave_info->send_index;
        int is_input = (function == AGILE_MODBUS_FC_READ_DISCRETE_INPUTS);

        for (int now_address = address; now_address < address + nb; now_address++) {
            int index = now_address - address;
            if (now_address >= 0 && now_address < TAB_MAX_NUM)
                agile_modbus_slave_io_set(ctx->send_buf + send_index, index,
                                          is_input ? _tab_input_bits[index] : _tab_bits[index]);
        }
    } break;

    case AGILE_MODBUS_FC_READ_HOLDING_REGISTERS:
    case AGILE_MODBUS_FC_READ_INPUT_REGISTERS: {
        int address = slave_info->address;
        int nb = slave_info->nb;
        int send_index = slave_info->send_index;
        int is_input = (function == AGILE_MODBUS_FC_READ_INPUT_REGISTERS);

        for (int now_address = address; now_address < address + nb; now_address++) {
            int index = now_address - address;
            if (now_address >= 0 && now_address < TAB_MAX_NUM)
                agile_modbus_slave_register_set(ctx->send_buf + send_index, index,
                                                is_input ? _tab_input_registers[index] : _tab_registers[index]);
        }
    } break;

    case AGILE_MODBUS_FC_WRITE_SINGLE_COIL:
    case AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS: {
        int address = slave_info->address;

        if (function == AGILE_MODBUS_FC_WRITE_SINGLE_COIL) {
            if (address >= 0 && address < TAB_MAX_NUM) {
                int data = *((int *)slave_info->buf);
                _tab_bits[address] = data;
            }
        } else {
            int nb = slave_info->nb;
            for (int now_address = address; now_address < address + nb; now_address++) {
                int index = now_address - address;
                if (now_address >= 0 && now_address < TAB_MAX_NUM) {
                    uint8_t status = agile_modbus_slave_io_get(slave_info->buf, index);
                    _tab_bits[index] = status;
                }
            }
        }
    } break;

    case AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER:
    case AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS: {
        int address = slave_info->address;
        if (function == AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER) {
            if (address >= 0 && address < TAB_MAX_NUM) {
                int data = *((int *)slave_info->buf);
                _tab_registers[address] = data;
            }
        } else {
            int nb = slave_info->nb;
            for (int now_address = address; now_address < address + nb; now_address++) {
                int index = now_address - address;
                if (now_address >= 0 && now_address < TAB_MAX_NUM) {
                    uint16_t data = agile_modbus_slave_register_get(slave_info->buf, index);
                    _tab_registers[index] = data;
                }
            }
        }

    } break;

    default:
        return -AGILE_MODBUS_EXCEPTION_ILLEGAL_FUNCTION;
    }

    return 0;
}

extern int rtu_slave_init(const char *dev, pthread_t *tid);
extern int tcp_slave_init(int port, pthread_t *tid);

int main(int argc, char *argv[])
{
    if (argc < 3) {
        LOG_E("Please enter ModbusSlave [dev] [port]!");
        return -1;
    }

    pthread_mutex_init(&_mtx, NULL);

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
