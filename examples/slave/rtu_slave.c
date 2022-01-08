#include "serial.h"
#include "slave.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME "rtu_slave"
#define DBG_LEVEL        DBG_LOG
#include "dbg_log.h"

static int _fd = -1;
static struct termios _old_tios = {0};

static void *rtu_entry(void *param)
{
    uint8_t ctx_send_buf[AGILE_MODBUS_MAX_ADU_LENGTH];
    uint8_t ctx_read_buf[AGILE_MODBUS_MAX_ADU_LENGTH];

    agile_modbus_rtu_t ctx_rtu;
    agile_modbus_t *ctx = &ctx_rtu._ctx;
    agile_modbus_rtu_init(&ctx_rtu, ctx_send_buf, sizeof(ctx_send_buf), ctx_read_buf, sizeof(ctx_read_buf));
    agile_modbus_set_slave(ctx, 1);

    LOG_I("Running.");

    while (1) {
        int read_len = serial_receive(_fd, ctx->read_buf, ctx->read_bufsz, 1000);
        if (read_len < 0) {
            LOG_E("Receive error, now exit.");
            break;
        }

        if (read_len == 0)
            continue;

        int send_len = agile_modbus_slave_handle(ctx, read_len, 0, slave_callback, NULL);
        serial_flush(_fd);
        if (send_len > 0)
            serial_send(_fd, ctx->send_buf, send_len);
    }

    serial_close(_fd, &_old_tios);
}

int rtu_slave_init(const char *dev, pthread_t *tid)
{
    _fd = serial_init(dev, 9600, 'N', 8, 1, &_old_tios);
    if (_fd < 0) {
        LOG_E("Open %s failed!", dev);
        return -1;
    }

    pthread_create(tid, NULL, rtu_entry, NULL);

    return 0;
}
