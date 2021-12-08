#include "agile_modbus.h"
#include "tcp.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME "tcp_master"
#define DBG_LEVEL        DBG_LOG
#include "dbg_log.h"

static int _sock = -1;

static void *cycle_entry(void *param)
{
    uint8_t ctx_send_buf[AGILE_MODBUS_MAX_ADU_LENGTH];
    uint8_t ctx_read_buf[AGILE_MODBUS_MAX_ADU_LENGTH];
    uint16_t hold_register[10];

    agile_modbus_tcp_t ctx_tcp;
    agile_modbus_t *ctx = &ctx_tcp._ctx;
    agile_modbus_tcp_init(&ctx_tcp, ctx_send_buf, sizeof(ctx_send_buf), ctx_read_buf, sizeof(ctx_read_buf));
    agile_modbus_set_slave(ctx, 1);

    LOG_I("Running.");

    while (1) {
        usleep(100000);

        tcp_flush(_sock);
        int send_len = agile_modbus_serialize_read_registers(ctx, 0, 10);
        tcp_send(_sock, ctx->send_buf, send_len);
        int read_len = tcp_receive(_sock, ctx->read_buf, ctx->read_bufsz, 1000);
        if (read_len < 0) {
            LOG_E("Receive error, now exit.");
            break;
        }

        if (read_len == 0) {
            LOG_W("Receive timeout.");
            continue;
        }

        int rc = agile_modbus_deserialize_read_registers(ctx, read_len, hold_register);
        if (rc < 0) {
            LOG_W("Receive failed.");
            if (rc != -1)
                LOG_W("Error code:%d", -128 - rc);

            continue;
        }

        LOG_I("Hold Registers:");
        for (int i = 0; i < 10; i++)
            LOG_I("Register [%d]: 0x%04X", i, hold_register[i]);

        printf("\r\n\r\n\r\n");
    }

    tcp_close(_sock);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        LOG_E("Please enter TcpMaster [ip] [port]!");
        return -1;
    }

    _sock = tcp_connect(argv[1], atoi(argv[2]));
    if (_sock == -1) {
        LOG_E("Connect %s:%d failed!", argv[1], atoi(argv[2]));
        return -1;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, cycle_entry, NULL);
    pthread_join(tid, NULL);
}
