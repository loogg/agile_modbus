#include "agile_modbus.h"
#include "serial.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME "broadcast_master"
#define DBG_LEVEL        DBG_LOG
#include "dbg_log.h"

static int _fd = -1;
static struct termios _old_tios = {0};
static const uint8_t _dirty_buf[100] = {1, 2, 3, 4, 5};

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

static char *normalize_path(char *fullpath)
{
    char *dst0, *dst, *src;

    src = fullpath;
    dst = fullpath;

    dst0 = dst;
    while (1) {
        char c = *src;

        if (c == '.') {
            if (!src[1])
                src++; /* '.' and ends */
            else if (src[1] == '/') {
                /* './' case */
                src += 2;

                while ((*src == '/') && (*src != '\0'))
                    src++;
                continue;
            } else if (src[1] == '.') {
                if (!src[2]) {
                    /* '..' and ends case */
                    src += 2;
                    goto up_one;
                } else if (src[2] == '/') {
                    /* '../' case */
                    src += 3;

                    while ((*src == '/') && (*src != '\0'))
                        src++;
                    goto up_one;
                }
            }
        }

        /* copy up the next '/' and erase all '/' */
        while ((c = *src++) != '\0' && c != '/')
            *dst++ = c;

        if (c == '/') {
            *dst++ = '/';
            while (c == '/')
                c = *src++;

            src--;
        } else if (!c)
            break;

        continue;

    up_one:
        dst--;
        if (dst < dst0)
            return NULL;
        while (dst0 < dst && dst[-1] != '/')
            dst--;
    }

    *dst = '\0';

    /* remove '/' in the end of path if exist */
    dst--;
    if ((dst != fullpath) && (*dst == '/'))
        *dst = '\0';

    return fullpath;
}

static int trans_file(int slave, char *file_path)
{
    uint8_t ctx_send_buf[2048];
    uint8_t ctx_read_buf[50];
    uint8_t raw_req[2048];
    int raw_req_len = 0;

    agile_modbus_rtu_t ctx_rtu;
    agile_modbus_t *ctx = &ctx_rtu._ctx;
    agile_modbus_rtu_init(&ctx_rtu, ctx_send_buf, sizeof(ctx_send_buf), ctx_read_buf, sizeof(ctx_read_buf));
    agile_modbus_set_slave(ctx, slave);
    agile_modbus_set_compute_meta_length_after_function_cb(ctx, compute_meta_length_after_function_callback);
    agile_modbus_set_compute_data_length_after_meta_cb(ctx, compute_data_length_after_meta_callback);

    if (normalize_path(file_path) == NULL)
        return -1;

    const char *file_name = file_path;
    while (1) {
        const char *ptr = strchr(file_name, '/');
        if (ptr == NULL)
            break;

        file_name = ptr + 1;
    }

    struct stat s;
    if (stat(file_path, &s) != 0)
        return -1;

    if (!S_ISREG(s.st_mode))
        return -1;

    int file_size = s.st_size;

    LOG_I("file name:%s, file size:%d", file_name, file_size);
    printf("\r\n\r\n");

    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL)
        return -1;

    int ret = 0;
    int write_file_size = 0;
    int step = 0;

    raw_req[0] = slave;
    raw_req[1] = AGILE_MODBUS_FC_TRANS_FILE;

    while (1) {
        usleep(5000);

        raw_req_len = 2;

        switch (step) {
        case 0: {
            step = 1;
            raw_req[raw_req_len++] = ((uint16_t)TRANS_FILE_CMD_START >> 8);
            raw_req[raw_req_len++] = ((uint16_t)TRANS_FILE_CMD_START & 0xFF);
            int nb = 5 + strlen(file_name);
            raw_req[raw_req_len++] = nb >> 8;
            raw_req[raw_req_len++] = nb & 0xFF;
            raw_req[raw_req_len++] = (file_size >> 24) & 0xFF;
            raw_req[raw_req_len++] = (file_size >> 16) & 0xFF;
            raw_req[raw_req_len++] = (file_size >> 8) & 0xFF;
            raw_req[raw_req_len++] = file_size & 0xFF;
            memcpy(raw_req + raw_req_len, file_name, strlen(file_name));
            raw_req_len += strlen(file_name);
            raw_req[raw_req_len++] = '\0';
        } break;

        case 1: {
            raw_req[raw_req_len++] = ((uint16_t)TRANS_FILE_CMD_DATA >> 8);
            raw_req[raw_req_len++] = ((uint16_t)TRANS_FILE_CMD_DATA & 0xFF);
            int nb_pos = raw_req_len;
            raw_req_len += 3;
            int recv_bytes = fread(raw_req + raw_req_len, 1, 1024, fp);
            raw_req_len += recv_bytes;
            write_file_size += recv_bytes;
            int nb = recv_bytes + 1;
            raw_req[nb_pos] = nb >> 8;
            raw_req[nb_pos + 1] = nb & 0xFF;
            if (recv_bytes < 1024) {
                raw_req[nb_pos + 2] = TRANS_FILE_FLAG_END;
                step = 2;
            } else
                raw_req[nb_pos + 2] = TRANS_FILE_FLAG_NOT_END;
        } break;

        default:
            break;
        }

        if (ret < 0)
            break;

        serial_flush(_fd);
        int send_len = agile_modbus_serialize_raw_request(ctx, raw_req, raw_req_len);
        serial_send(_fd, ctx->send_buf, send_len);

        //脏数据
        serial_send(_fd, _dirty_buf, sizeof(_dirty_buf));

        print_progress(write_file_size, file_size);

        if (step == 2)
            break;
    }

    fclose(fp);
    fp = NULL;

    printf("\r\n\r\n");

    return ret;
}

static void *cycle_entry(void *param)
{
    char tmp[100];

    while (1) {
        printf("please enter file_path:\r\n");
        fgets(tmp, sizeof(tmp), stdin);
        for (int i = 0; i < strlen(tmp); i++) {
            if (tmp[i] == '\r' || tmp[i] == '\n') {
                tmp[i] = '\0';
                break;
            }
        }

        trans_file(0, tmp);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        LOG_E("Please enter broadcast_master [dev]!");
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
}
