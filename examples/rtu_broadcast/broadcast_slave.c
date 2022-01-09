#include "serial.h"
#include "agile_modbus.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include "ringbuffer.h"

#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME "broadcast_slave"
#define DBG_LEVEL        DBG_LOG
#include "dbg_log.h"

static int _fd = -1;
static struct termios _old_tios = {0};
static FILE *_fp = NULL;
static int _slave = 0;
static int _file_size = 0;
static int _write_file_size = 0;
static pthread_mutex_t _mtx;
static sem_t _notice;
static struct rt_ringbuffer _recv_rb;
static uint8_t _recv_rb_buf[20480];

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
        snprintf(own_file_name, sizeof(own_file_name), "%d_%s", ctx->slave, file_name);

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

static void *recv_entry(void *param)
{
    uint8_t tmp[4096];
    while (1) {
        int read_len = serial_receive(_fd, tmp, sizeof(tmp), 1000);

        while (read_len > 0) {
            pthread_mutex_lock(&_mtx);
            int rb_recv_len = rt_ringbuffer_put(&_recv_rb, tmp, read_len);
            pthread_mutex_unlock(&_mtx);

            read_len -= rb_recv_len;

            if (rb_recv_len > 0)
                sem_post(&_notice);
            else
                usleep(1000);
        }
    }
}

static int rb_receive(uint8_t *buf, int bufsz, int timeout)
{
    int len = 0;

    while (1) {
        while (sem_trywait(&_notice) == 0)
            ;
        pthread_mutex_lock(&_mtx);
        int read_len = rt_ringbuffer_get(&_recv_rb, buf + len, bufsz);
        pthread_mutex_unlock(&_mtx);

        if (read_len > 0) {
            len += read_len;
            bufsz -= read_len;
            if (bufsz == 0)
                break;

            continue;
        }

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout / 1000;
        ts.tv_nsec += (timeout % 1000) * 1000;
        if (sem_timedwait(&_notice, &ts) != 0)
            break;
    }

    return len;
}

static void *cycle_entry(void *param)
{
    uint8_t ctx_send_buf[50];
    uint8_t ctx_read_buf1[2048];
    uint8_t ctx_read_buf2[2048];
    uint8_t *used_buf_ptr = ctx_read_buf1;

    int frame_length = 0;
    int remain_length = 0;

    agile_modbus_rtu_t ctx_rtu;
    agile_modbus_t *ctx = &ctx_rtu._ctx;
    agile_modbus_rtu_init(&ctx_rtu, ctx_send_buf, sizeof(ctx_send_buf), used_buf_ptr, sizeof(ctx_read_buf1));
    agile_modbus_set_slave(ctx, _slave);
    agile_modbus_set_compute_meta_length_after_function_cb(ctx, compute_meta_length_after_function_callback);
    agile_modbus_set_compute_data_length_after_meta_cb(ctx, compute_data_length_after_meta_callback);

    LOG_I("slave %d running.", _slave);

    while (1) {
        int read_len = rb_receive(ctx->read_buf + remain_length, ctx->read_bufsz - remain_length, 1000);

        int total_len = read_len + remain_length;

        int is_reset = 0;

        if (total_len && read_len == 0)
            is_reset = 1;

        // 解包，为了防止脏数据，不能直接丢，往后挪一个字节继续解析
        while (total_len > 0) {
            int rc = agile_modbus_slave_handle(ctx, total_len, 1, slave_callback, &frame_length);
            if (rc >= 0) {
                ctx->read_buf = ctx->read_buf + frame_length;
                ctx->read_bufsz = ctx->read_bufsz - frame_length;

                remain_length = total_len - frame_length;
                total_len = remain_length;
            } else {

                if (total_len > 1600 || is_reset) {
                    ctx->read_buf++;
                    ctx->read_bufsz--;
                    total_len--;
                    continue;
                }

                if (used_buf_ptr == ctx_read_buf1) {
                    memcpy(ctx_read_buf2, ctx->read_buf, total_len);

                    ctx->read_buf = ctx_read_buf2;
                    ctx->read_bufsz = sizeof(ctx_read_buf2);
                    used_buf_ptr = ctx_read_buf2;
                } else {
                    memcpy(ctx_read_buf1, ctx->read_buf, total_len);

                    ctx->read_buf = ctx_read_buf1;
                    ctx->read_bufsz = sizeof(ctx_read_buf1);
                    used_buf_ptr = ctx_read_buf1;
                }

                remain_length = total_len;

                break;
            }
        }

        if (total_len == 0) {
            remain_length = 0;

            ctx->read_buf = ctx_read_buf1;
            ctx->read_bufsz = sizeof(ctx_read_buf1);
            used_buf_ptr = ctx_read_buf1;
        }
    }

    serial_close(_fd, &_old_tios);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        LOG_E("Please enter broadcast_slave [dev] [slave]!");
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

    pthread_mutex_init(&_mtx, NULL);
    sem_init(&_notice, 0, 0);
    rt_ringbuffer_init(&_recv_rb, _recv_rb_buf, sizeof(_recv_rb_buf));

    pthread_t tid1, tid2;
    pthread_create(&tid1, NULL, cycle_entry, NULL);
    pthread_create(&tid2, NULL, recv_entry, NULL);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    return 0;
}
