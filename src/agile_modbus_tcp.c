#include "agile_modbus.h"
#include "agile_modbus_tcp.h"

static int agile_modbus_tcp_set_slave(agile_modbus_t *ctx, int slave)
{
    ctx->slave = slave;
    return 0;
}

/* Builds a TCP request header */
static int agile_modbus_tcp_build_request_basis(agile_modbus_t *ctx, int function,
                                                int addr, int nb,
                                                uint8_t *req)
{
    agile_modbus_tcp_t *ctx_tcp = ctx->backend_data;

    /* Increase transaction ID */
    if (ctx_tcp->t_id < UINT16_MAX)
        ctx_tcp->t_id++;
    else
        ctx_tcp->t_id = 0;
    req[0] = ctx_tcp->t_id >> 8;
    req[1] = ctx_tcp->t_id & 0x00ff;

    /* Protocol Modbus */
    req[2] = 0;
    req[3] = 0;

    /* Length will be defined later by set_req_length_tcp at offsets 4
       and 5 */

    req[6] = ctx->slave;
    req[7] = function;
    req[8] = addr >> 8;
    req[9] = addr & 0x00ff;
    req[10] = nb >> 8;
    req[11] = nb & 0x00ff;

    return AGILE_MODBUS_TCP_PRESET_REQ_LENGTH;
}

/* Builds a TCP response header */
static int agile_modbus_tcp_build_response_basis(agile_modbus_sft_t *sft, uint8_t *rsp)
{
    /* Extract from MODBUS Messaging on TCP/IP Implementation
       Guide V1.0b (page 23/46):
       The transaction identifier is used to associate the future
       response with the request. */
    rsp[0] = sft->t_id >> 8;
    rsp[1] = sft->t_id & 0x00ff;

    /* Protocol Modbus */
    rsp[2] = 0;
    rsp[3] = 0;

    /* Length will be set later by send_msg (4 and 5) */

    /* The slave ID is copied from the indication */
    rsp[6] = sft->slave;
    rsp[7] = sft->function;

    return AGILE_MODBUS_TCP_PRESET_RSP_LENGTH;
}

static int agile_modbus_tcp_prepare_response_tid(const uint8_t *req, int *req_length)
{
    return (req[0] << 8) + req[1];
}

static int agile_modbus_tcp_send_msg_pre(uint8_t *req, int req_length)
{
    /* Substract the header length to the message length */
    int mbap_length = req_length - 6;

    req[4] = mbap_length >> 8;
    req[5] = mbap_length & 0x00FF;

    return req_length;
}

static int agile_modbus_tcp_check_integrity(agile_modbus_t *ctx, uint8_t *msg, const int msg_length)
{
    return msg_length;
}

static int agile_modbus_tcp_pre_check_confirmation(agile_modbus_t *ctx, const uint8_t *req,
                                                   const uint8_t *rsp, int rsp_length)
{
    /* Check transaction ID */
    if (req[0] != rsp[0] || req[1] != rsp[1])
        return -1;

    /* Check protocol ID */
    if (rsp[2] != 0x0 && rsp[3] != 0x0)
        return -1;

    return 0;
}

static const agile_modbus_backend_t agile_modbus_tcp_backend = 
{
    AGILE_MODBUS_BACKEND_TYPE_TCP,
    AGILE_MODBUS_TCP_HEADER_LENGTH,
    AGILE_MODBUS_TCP_CHECKSUM_LENGTH,
    AGILE_MODBUS_TCP_MAX_ADU_LENGTH,
    agile_modbus_tcp_set_slave,
    agile_modbus_tcp_build_request_basis,
    agile_modbus_tcp_build_response_basis,
    agile_modbus_tcp_prepare_response_tid,
    agile_modbus_tcp_send_msg_pre,
    agile_modbus_tcp_check_integrity,
    agile_modbus_tcp_pre_check_confirmation
};

int agile_modbus_tcp_init(agile_modbus_tcp_t *ctx, uint8_t *send_buf, int send_bufsz, uint8_t *read_buf, int read_bufsz)
{
    agile_modbus_common_init(&(ctx->_ctx), send_buf, send_bufsz, read_buf, read_bufsz);
    ctx->_ctx.backend = &agile_modbus_tcp_backend;
    ctx->_ctx.backend_data = ctx;

    ctx->t_id = 0;

    return 0;
}
