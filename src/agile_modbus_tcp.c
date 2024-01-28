/**
 * @file    agile_modbus_tcp.c
 * @brief   Agile Modbus package TCP source file
 * @author  Ma Longwei (2544047213@qq.com)
 * @date    2021-12-02
 *
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 Ma Longwei.
 * All rights reserved.</center></h2>
 *
 */

#include "agile_modbus.h"

#if AGILE_MODBUS_USING_TCP

#include "agile_modbus_tcp.h"

/** @defgroup TCP TCP
 * @{
 */

/** @defgroup TCP_Private_Functions TCP Private Functions
 * @{
 */

/**
 * @brief   TCP set address interface
 * @param   ctx modbus handle
 * @param   slave slave address
 * @return  0: success
 */
static int agile_modbus_tcp_set_slave(agile_modbus_t *ctx, int slave)
{
    ctx->slave = slave;
    return 0;
}

/**
 *@brief   TCP builds the basic request message interface (header message)
 *@param   ctx modbus handle
 *@param   function function code
 *@param   addr register address
 *@param   nb number of registers
 *@param   req data storage pointer
 *@return  data length
 */
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

/**
 * @brief   TCP builds a basic response message interface (header message)
 * @param   sft modbus header parameter structure pointer
 * @param   rsp data storage pointer
 * @return  data length
 */
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

/**
 * @brief   TCP ready response interface
 * @param   req request data pointer
 * @param   req_length request data length
 * @return  transaction identifier
 */
static int agile_modbus_tcp_prepare_response_tid(const uint8_t *req, int *req_length)
{
    (void)req_length;
    return (req[0] << 8) + req[1];
}

/**
 * @brief   TCP pre-send data interface (calculate the value of the length field and store it)
 * @param   req data storage pointer
 * @param   req_length existing data length
 * @return  data length
 */
static int agile_modbus_tcp_send_msg_pre(uint8_t *req, int req_length)
{
    /* Substract the header length to the message length */
    int mbap_length = req_length - 6;

    req[4] = mbap_length >> 8;
    req[5] = mbap_length & 0x00FF;

    return req_length;
}

/**
 * @brief   TCP check receiving data integrity interface
 * @param   ctx modbus handle
 * @param   msg Receive data pointer
 * @param   msg_length valid data length
 * @return  valid data length
 */
static int agile_modbus_tcp_check_integrity(agile_modbus_t *ctx, uint8_t *msg, const int msg_length)
{
    (void)ctx;
    (void)msg;
    return msg_length;
}

/**
 * @brief   TCP pre-check confirmation interface (compare transaction identifier and protocol identifier)
 * @param   ctx modbus handle
 * @param   req request data pointer
 * @param   rsp response data pointer
 * @param   rsp_length response data length
 * @return  0: success; others: exception
 */
static int agile_modbus_tcp_pre_check_confirmation(agile_modbus_t *ctx, const uint8_t *req,
                                                   const uint8_t *rsp, int rsp_length)
{
    (void)ctx;
    (void)rsp_length;
    /* Check transaction ID */
    if (req[0] != rsp[0] || req[1] != rsp[1])
        return -1;

    /* Check protocol ID */
    if (rsp[2] != 0x0 && rsp[3] != 0x0)
        return -1;

    return 0;
}

/**
 * @}
 */

/** @defgroup TCP_Private_Constants TCP Private Constants
 * @{
 */

/**
 * @brief   TCP backend interface
 */
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
        agile_modbus_tcp_pre_check_confirmation};

/**
 * @}
 */

/** @defgroup TCP_Exported_Functions TCP Exported Functions
 * @{
 */

/**
 * @brief   TCP initialization
 * @param   ctx TCP handle
 * @param   send_buf send buffer
 * @param   send_bufsz send buffer size
 * @param   read_buf receive buffer
 * @param   read_bufsz receive buffer size
 * @return  0: success
 */
int agile_modbus_tcp_init(agile_modbus_tcp_t *ctx, uint8_t *send_buf, int send_bufsz, uint8_t *read_buf, int read_bufsz)
{
    agile_modbus_common_init(&(ctx->_ctx), send_buf, send_bufsz, read_buf, read_bufsz);
    ctx->_ctx.backend = &agile_modbus_tcp_backend;
    ctx->_ctx.backend_data = ctx;

    ctx->t_id = 0;

    return 0;
}

/**
 * @}
 */

/**
 * @}
 */

#endif /* AGILE_MODBUS_USING_TCP */
