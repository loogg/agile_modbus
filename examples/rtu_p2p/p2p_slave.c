#include "serial.h"
#include "agile_modbus.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME "p2p_slave"
#define DBG_LEVEL        DBG_LOG
#include "dbg_log.h"

static int _fd = -1;
static struct termios _old_tios = {0};
static FILE *_fp = NULL;
static int _slave = 0;
static int _file_size = 0;
static int _write_file_size = 0;

#define AGILE_MODBUS_FC_TRANS_FILE 0x50
#define TRANS_FILE_CMD_START       0x0001
#define TRANS_FILE_CMD_DATA        0x0002
#define TRANS_FILE_FLAG_END        0x00
#define TRANS_FILE_FLAG_NOT_END    0x01

static uint8_t compute_meta_length_after_function_callback(agile_modbus_t *ctx, int function,
                                                           agile_modbus_msg_type_t msg_type)
{
    int length;

    if (msg_type == AGILE_MODBUS_MSG_INDICATION) {
        length = 0;
        if (function == AGILE_MODBUS_FC_TRANS_FILE)
            length = 4;
    } else {
        /* MSG_CONFIRMATION */
        length = 1;
        if (function == AGILE_MODBUS_FC_TRANS_FILE)
            length = 3;
    }

    return length;
}

static int compute_data_length_after_meta_callback(agile_modbus_t *ctx, uint8_t *msg,
                                                   int msg_length, agile_modbus_msg_type_t msg_type)
{
    int function = msg[ctx->backend->header_length];
    int length;

    if (msg_type == AGILE_MODBUS_MSG_INDICATION) {
        length = 0;
        if (function == AGILE_MODBUS_FC_TRANS_FILE)
            length = (msg[ctx->backend->header_length + 3] << 8) + msg[ctx->backend->header_length + 4];
    } else {
        /* MSG_CONFIRMATION */
        length = 0;
    }

    return length;
}

static void print_progress(size_t cur_size, size_t total_size)
{
    static uint8_t progress_sign[100 + 1];
    uint8_t i, per = cur_size * 100 / total_size;

    if (per > 100) {
        per = 100;
    }

    for (i = 0; i < 100; i++) {
        if (i < per) {
            progress_sign[i] = '=';
        } else if (per == i) {
            progress_sign[i] = '>';
        } else {
            progress_sign[i] = ' ';
        }
    }

    progress_sign[sizeof(progress_sign) - 1] = '\0';

    LOG_I("\033[2A");
    LOG_I("Trans: [%s] %d%%", progress_sign, per);
}

/**
 * @brief   从机回调函数
 * @param   ctx modbus 句柄
 * @param   slave_info 从机信息体
 * @return  =0:正常;
 *          <0:异常
 *             (-AGILE_MODBUS_EXCEPTION_UNKNOW(-255): 未知异常，从机不会打包响应数据)
 *             (其他负数异常码: 从机会打包异常响应数据)
 */
static int slave_callback(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info)
{
    int function = slave_info->sft->function;

    if (function != AGILE_MODBUS_FC_TRANS_FILE)
        return 0;

    int ret = 0;

    int send_index = slave_info->send_index;
    int data_len = slave_info->nb;
    uint8_t *data_ptr = slave_info->buf;
    int cmd = (data_ptr[0] << 8) + data_ptr[1];
    int cmd_data_len = (data_ptr[2] << 8) + data_ptr[3];
    uint8_t *cmd_data_ptr = data_ptr + 4;

    switch (cmd) {
    case TRANS_FILE_CMD_START: {
        if (_fp != NULL) {
            LOG_W("_fp is not NULL, now close _fp.");
            fclose(_fp);
            _fp = NULL;
            ret = -1;
            break;
        }

        if (cmd_data_len <= 4) {
            LOG_W("cmd start date_len must be greater than 4.");
            ret = -1;
            break;
        }

        _file_size = (((int)cmd_data_ptr[0] << 24) +
                      ((int)cmd_data_ptr[1] << 16) +
                      ((int)cmd_data_ptr[2] << 8) +
                      (int)cmd_data_ptr[3]);

        _write_file_size = 0;

        char *file_name = (char *)(data_ptr + 8);
        if (strlen(file_name) >= 256) {
            LOG_W("file name must be less than 256.");
            ret = -1;
            break;
        }

        char own_file_name[300];
        snprintf(own_file_name, sizeof(own_file_name), "%d_%s", slave_info->sft->slave, file_name);

        LOG_I("write to %s, file size is %d", own_file_name, _file_size);
        printf("\r\n\r\n");

        _fp = fopen(own_file_name, "wb");
        if (_fp == NULL) {
            LOG_W("open file %s error.", own_file_name);
            ret = -1;
            break;
        }
    } break;

    case TRANS_FILE_CMD_DATA: {
        if (_fp == NULL) {
            LOG_W("_fp is NULL.");
            ret = -1;
            break;
        }

        if (cmd_data_len <= 0) {
            LOG_W("cmd data data_len must be greater than 0");
            ret = -1;
            break;
        }

        int flag = cmd_data_ptr[0];
        int file_len = cmd_data_len - 1;
        if (file_len > 0) {
            if (fwrite(cmd_data_ptr + 1, file_len, 1, _fp) != 1) {
                LOG_W("write to file error.");
                ret = -1;
                break;
            }
        }
        _write_file_size += file_len;

        print_progress(_write_file_size, _file_size);

        if (flag == TRANS_FILE_FLAG_END) {
            fclose(_fp);
            _fp = NULL;
            printf("\r\n\r\n");
            if (_write_file_size != _file_size) {
                LOG_W("_write_file_size (%d) != _file_size (%d)", _write_file_size, _file_size);
                ret = -1;
                break;
            }

            LOG_I("success.");
        }

    } break;

    default:
        ret = -1;
        break;
    }

    ctx->send_buf[send_index++] = data_ptr[0];
    ctx->send_buf[send_index++] = data_ptr[1];
    ctx->send_buf[send_index++] = (ret == 0) ? 0x01 : 0x00;
    *(slave_info->rsp_length) = send_index;

    return 0;
}

static void *cycle_entry(void *param)
{
    uint8_t ctx_send_buf[50];
    uint8_t ctx_read_buf[2048];

    agile_modbus_rtu_t ctx_rtu;
    agile_modbus_t *ctx = &ctx_rtu._ctx;
    agile_modbus_rtu_init(&ctx_rtu, ctx_send_buf, sizeof(ctx_send_buf), ctx_read_buf, sizeof(ctx_read_buf));
    agile_modbus_set_slave(ctx, _slave);
    agile_modbus_set_compute_meta_length_after_function_cb(ctx, compute_meta_length_after_function_callback);
    agile_modbus_set_compute_data_length_after_meta_cb(ctx, compute_data_length_after_meta_callback);

    LOG_I("slave %d running.", _slave);

    while (1) {
        int read_len = serial_receive(_fd, ctx->read_buf, ctx->read_bufsz, 1000);
        if (read_len < 0) {
            LOG_E("Receive error, now exit.");
            break;
        }

        if (read_len == 0)
            continue;

        int send_len = agile_modbus_slave_handle(ctx, read_len, 1, slave_callback, NULL);
        serial_flush(_fd);
        if (send_len > 0)
            serial_send(_fd, ctx->send_buf, send_len);
    }

    serial_close(_fd, &_old_tios);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        LOG_E("Please enter p2p_slave [dev] [slave]!");
        return -1;
    }

    _slave = atoi(argv[2]);
    if (_slave <= 0) {
        LOG_E("slave must be greater than 0!");
        return -1;
    }

    _fd = serial_init(argv[1], 115200, 'N', 8, 1, &_old_tios);
    if (_fd < 0) {
        LOG_E("Open %s failed!", argv[1]);
        return -1;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, cycle_entry, NULL);
    pthread_join(tid, NULL);

    return 0;
}
