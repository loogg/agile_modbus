/**
 * @file    agile_modbus.c
 * @brief   Agile Modbus 软件包通用源文件
 * @author  马龙伟 (2544047213@qq.com)
 * @version 1.1.0
 * @date    2021-12-02
 *
 @verbatim
    使用：
    用户需要实现硬件接口的 `发送数据` 、 `等待数据接收结束` 、 `清空接收缓存` 函数

    - 主机：
        1. agile_modbus_rtu_init / agile_modbus_tcp_init 初始化 RTU/TCP 环境
        2. agile_modbus_set_slave 设置从机地址
        3. `清空接收缓存`
        4. `agile_modbus_serialize_xxx` 打包请求数据
        5. `发送数据`
        6. `等待数据接收结束`
        7. `agile_modbus_deserialize_xxx` 解析响应数据
        8. 用户处理得到的数据

 @endverbatim
 *
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 Ma Longwei.
 * All rights reserved.</center></h2>
 *
 */

#include "agile_modbus.h"
#include <string.h>

/** @defgroup COMMON Common
 * @{
 */

/** @defgroup COMMON_Private_Constants Common Private Constants
 * @{
 */
#define AGILE_MODBUS_MSG_LENGTH_UNDEFINED -1 /**< 对应功能码数据长度未定义 */
/**
 * @}
 */

/** @defgroup COMMON_Private_Functions Common Private Functions
 * @{
 */

/**
 * @brief   计算功能码后要接收的数据元长度
 @verbatim
    ---------- Request     Indication ----------
    | Client | ---------------------->| Server |
    ---------- Confirmation  Response ----------

    以 03 功能码请求报文举例

    ---------- ------ --------------- ---------
    | header | | 03 | | 00 00 00 01 | | CRC16 |
    ---------- ------ --------------- ---------

    ----------
    | header |
    ----------
        RTU: 设备地址
        TCP: | 事务处理标识  协议标识  长度  单元标识符 |

    ---------------
    | 00 00 00 01 |
    ---------------
        数据元: 与功能码相关的数据，如 03 功能码数据元中包含寄存器起始地址和寄存器长度

 @endverbatim
 * @param   ctx modbus 句柄
 * @param   function 功能码
 * @param   msg_type 消息类型
 * @return  数据元长度
 */
static uint8_t agile_modbus_compute_meta_length_after_function(agile_modbus_t *ctx, int function, agile_modbus_msg_type_t msg_type)
{
    int length;

    if (msg_type == AGILE_MODBUS_MSG_INDICATION) {
        if (function <= AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER) {
            length = 4;
        } else if (function == AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS ||
                   function == AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS) {
            length = 5;
        } else if (function == AGILE_MODBUS_FC_MASK_WRITE_REGISTER) {
            length = 6;
        } else if (function == AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS) {
            length = 9;
        } else {
            /* MODBUS_FC_READ_EXCEPTION_STATUS, MODBUS_FC_REPORT_SLAVE_ID */
            length = 0;
            if (ctx->compute_meta_length_after_function)
                length = ctx->compute_meta_length_after_function(ctx, function, msg_type);
        }
    } else {
        /* MSG_CONFIRMATION */
        switch (function) {
        case AGILE_MODBUS_FC_READ_COILS:
        case AGILE_MODBUS_FC_READ_DISCRETE_INPUTS:
        case AGILE_MODBUS_FC_READ_HOLDING_REGISTERS:
        case AGILE_MODBUS_FC_READ_INPUT_REGISTERS:
        case AGILE_MODBUS_FC_REPORT_SLAVE_ID:
        case AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS:
            length = 1;
            break;

        case AGILE_MODBUS_FC_WRITE_SINGLE_COIL:
        case AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER:
        case AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS:
        case AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
            length = 4;
            break;

        case AGILE_MODBUS_FC_MASK_WRITE_REGISTER:
            length = 6;
            break;

        default:
            length = 1;
            if (ctx->compute_meta_length_after_function)
                length = ctx->compute_meta_length_after_function(ctx, function, msg_type);
        }
    }

    return length;
}

/**
 * @brief   计算数据元之后要接收的数据长度
 @verbatim
    ---------- Request     Indication ----------
    | Client | ---------------------->| Server |
    ---------- Confirmation  Response ----------

    以 03 功能码响应报文举例

    ---------- ------ ------ --------- ---------
    | header | | 03 | | 02 | | 00 00 | | CRC16 |
    ---------- ------ ------ --------- ---------

    ----------
    | header |
    ----------
        RTU: 设备地址
        TCP: | 事务处理标识  协议标识  长度  单元标识符 |

    ------
    | 02 |
    ------
        数据元: 两个字节数据

    ---------
    | 00 00 |
    ---------
        数据

 @endverbatim
 * @param   ctx modbus 句柄
 * @param   msg 消息指针
 * @param   msg_length 消息长度
 * @param   msg_type 消息类型
 * @return  数据长度
 */
static int agile_modbus_compute_data_length_after_meta(agile_modbus_t *ctx, uint8_t *msg, int msg_length, agile_modbus_msg_type_t msg_type)
{
    int function = msg[ctx->backend->header_length];
    int length;

    if (msg_type == AGILE_MODBUS_MSG_INDICATION) {
        switch (function) {
        case AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS:
        case AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
            length = msg[ctx->backend->header_length + 5];
            break;

        case AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS:
            length = msg[ctx->backend->header_length + 9];
            break;

        default:
            length = 0;
            if (ctx->compute_data_length_after_meta)
                length = ctx->compute_data_length_after_meta(ctx, msg, msg_length, msg_type);
        }
    } else {
        /* MSG_CONFIRMATION */
        if (function <= AGILE_MODBUS_FC_READ_INPUT_REGISTERS ||
            function == AGILE_MODBUS_FC_REPORT_SLAVE_ID ||
            function == AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS) {
            length = msg[ctx->backend->header_length + 1];
        } else {
            length = 0;
            if (ctx->compute_data_length_after_meta)
                length = ctx->compute_data_length_after_meta(ctx, msg, msg_length, msg_type);
        }
    }

    length += ctx->backend->checksum_length;

    return length;
}

/**
 * @brief   检验接收数据正确性
 * @param   ctx modbus 句柄
 * @param   msg 消息指针
 * @param   msg_length 消息长度
 * @param   msg_type 消息类型
 * @return  >0:正确，modbus 数据帧长度; 其他:异常
 */
static int agile_modbus_receive_msg_judge(agile_modbus_t *ctx, uint8_t *msg, int msg_length, agile_modbus_msg_type_t msg_type)
{
    int remain_len = msg_length;

    if (remain_len > (int)ctx->backend->max_adu_length)
        return -1;
    remain_len -= (ctx->backend->header_length + 1);
    if (remain_len < 0)
        return -1;
    remain_len -= agile_modbus_compute_meta_length_after_function(ctx, msg[ctx->backend->header_length], msg_type);
    if (remain_len < 0)
        return -1;
    remain_len -= agile_modbus_compute_data_length_after_meta(ctx, msg, msg_length, msg_type);
    if (remain_len < 0)
        return -1;

    return ctx->backend->check_integrity(ctx, msg, msg_length - remain_len);
}

/**
 * @brief   计算预期响应数据长度
 * @note    如果是特殊的功能码，返回 AGILE_MODBUS_MSG_LENGTH_UNDEFINED ，但这不代表异常。
 *          agile_modbus_check_confirmation 调用该 API 处理时认为 AGILE_MODBUS_MSG_LENGTH_UNDEFINED 返回值也是有效的。
 * @param   ctx modbus 句柄
 * @param   req 请求数据指针
 * @return  预期响应数据长度
 */
static int agile_modbus_compute_response_length_from_request(agile_modbus_t *ctx, uint8_t *req)
{
    int length;
    const int offset = ctx->backend->header_length;

    switch (req[offset]) {
    case AGILE_MODBUS_FC_READ_COILS:
    case AGILE_MODBUS_FC_READ_DISCRETE_INPUTS: {
        /* Header + nb values (code from write_bits) */
        int nb = (req[offset + 3] << 8) | req[offset + 4];
        length = 2 + (nb / 8) + ((nb % 8) ? 1 : 0);
    } break;

    case AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS:
    case AGILE_MODBUS_FC_READ_HOLDING_REGISTERS:
    case AGILE_MODBUS_FC_READ_INPUT_REGISTERS:
        /* Header + 2 * nb values */
        length = 2 + 2 * (req[offset + 3] << 8 | req[offset + 4]);
        break;

    case AGILE_MODBUS_FC_WRITE_SINGLE_COIL:
    case AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER:
    case AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS:
    case AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
        length = 5;
        break;

    case AGILE_MODBUS_FC_MASK_WRITE_REGISTER:
        length = 7;
        break;

    default:
        /* The response is device specific (the header provides the
            length) */
        return AGILE_MODBUS_MSG_LENGTH_UNDEFINED;
    }

    return offset + length + ctx->backend->checksum_length;
}

/**
 * @brief   检查确认从机响应的数据
 * @param   ctx modbus 句柄
 * @param   req 请求数据指针
 * @param   rsp 响应数据指针
 * @param   rsp_length 响应数据长度
 * @return  >=0:对应功能码响应对象的长度(如 03 功能码，值代表寄存器个数);
 *          其他:异常 (-1：报文错误；其他：可根据 `-128 - $返回值` 得到异常码)
 */
static int agile_modbus_check_confirmation(agile_modbus_t *ctx, uint8_t *req,
                                           uint8_t *rsp, int rsp_length)
{
    int rc;
    int rsp_length_computed;
    const int offset = ctx->backend->header_length;
    const int function = rsp[offset];

    if (ctx->backend->pre_check_confirmation) {
        rc = ctx->backend->pre_check_confirmation(ctx, req, rsp, rsp_length);
        if (rc < 0)
            return -1;
    }

    rsp_length_computed = agile_modbus_compute_response_length_from_request(ctx, req);

    /* Exception code */
    if (function >= 0x80) {
        if (rsp_length == (offset + 2 + (int)ctx->backend->checksum_length) && req[offset] == (rsp[offset] - 0x80))
            return (-128 - rsp[offset + 1]);
        else
            return -1;
    }

    /* Check length */
    if ((rsp_length == rsp_length_computed || rsp_length_computed == AGILE_MODBUS_MSG_LENGTH_UNDEFINED) && function < 0x80) {
        int req_nb_value;
        int rsp_nb_value;

        /* Check function code */
        if (function != req[offset])
            return -1;

        /* Check the number of values is corresponding to the request */
        switch (function) {
        case AGILE_MODBUS_FC_READ_COILS:
        case AGILE_MODBUS_FC_READ_DISCRETE_INPUTS:
            /* Read functions, 8 values in a byte (nb
                * of values in the request and byte count in
                * the response. */
            req_nb_value = (req[offset + 3] << 8) + req[offset + 4];
            req_nb_value = (req_nb_value / 8) + ((req_nb_value % 8) ? 1 : 0);
            rsp_nb_value = rsp[offset + 1];
            break;

        case AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS:
        case AGILE_MODBUS_FC_READ_HOLDING_REGISTERS:
        case AGILE_MODBUS_FC_READ_INPUT_REGISTERS:
            /* Read functions 1 value = 2 bytes */
            req_nb_value = (req[offset + 3] << 8) + req[offset + 4];
            rsp_nb_value = (rsp[offset + 1] / 2);
            break;

        case AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS:
        case AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
            /* N Write functions */
            req_nb_value = (req[offset + 3] << 8) + req[offset + 4];
            rsp_nb_value = (rsp[offset + 3] << 8) | rsp[offset + 4];
            break;

        case AGILE_MODBUS_FC_REPORT_SLAVE_ID:
            /* Report slave ID (bytes received) */
            req_nb_value = rsp_nb_value = rsp[offset + 1];
            break;

        default:
            /* 1 Write functions & others */
            req_nb_value = rsp_nb_value = 1;
        }

        if (req_nb_value == rsp_nb_value)
            rc = rsp_nb_value;
        else
            rc = -1;
    } else
        rc = -1;

    return rc;
}

/**
 * @}
 */

/** @defgroup COMMON_Exported_Functions Common Exported Functions
 * @{
 */

/** @defgroup COMMON_Operation_Functions Common Operation Functions
 * @{
 */

/**
 * @brief   初始化 modbus 句柄
 * @param   ctx modbus 句柄
 * @param   send_buf 发送缓冲区
 * @param   send_bufsz 发送缓冲区大小
 * @param   read_buf 接收缓冲区
 * @param   read_bufsz 接收缓冲区大小
 */
void agile_modbus_common_init(agile_modbus_t *ctx, uint8_t *send_buf, int send_bufsz, uint8_t *read_buf, int read_bufsz)
{
    memset(ctx, 0, sizeof(agile_modbus_t));
    ctx->slave = -1;
    ctx->send_buf = send_buf;
    ctx->send_bufsz = send_bufsz;
    ctx->read_buf = read_buf;
    ctx->read_bufsz = read_bufsz;
}

/**
 * @brief   设置地址
 * @param   ctx modbus 句柄
 * @param   slave 地址
 * @return  0:成功
 */
int agile_modbus_set_slave(agile_modbus_t *ctx, int slave)
{
    return ctx->backend->set_slave(ctx, slave);
}

/**
 * @brief   设置 modbus 对象的计算功能码后要接收的数据元长度回调函数
 * @param   ctx modbus 句柄
 * @param   cb 计算功能码后要接收的数据元长度回调函数
 * @see     agile_modbus_compute_meta_length_after_function
 */
void agile_modbus_set_compute_meta_length_after_function_cb(agile_modbus_t *ctx,
                                                            uint8_t (*cb)(agile_modbus_t *ctx, int function,
                                                                          agile_modbus_msg_type_t msg_type))
{
    ctx->compute_meta_length_after_function = cb;
}

/**
 * @brief   设置 modbus 对象的计算数据元之后要接收的数据长度回调函数
 * @param   ctx modbus 句柄
 * @param   cb 计算数据元之后要接收的数据长度回调函数
 * @see     agile_modbus_compute_data_length_after_meta
 */
void agile_modbus_set_compute_data_length_after_meta_cb(agile_modbus_t *ctx,
                                                        int (*cb)(agile_modbus_t *ctx, uint8_t *msg,
                                                                  int msg_length, agile_modbus_msg_type_t msg_type))
{
    ctx->compute_data_length_after_meta = cb;
}

/**
 * @brief   校验接收数据正确性
 * @note    该 API 返回的是 modbus 数据帧长度，比如 8 个字节的 modbus 数据帧 + 2 个字节的脏数据，返回 8
 * @param   ctx modbus 句柄
 * @param   msg_length 接收数据长度
 * @param   msg_type 消息类型
 * @return  >0:正确，modbus 数据帧长度; 其他:异常
 */
int agile_modbus_receive_judge(agile_modbus_t *ctx, int msg_length, agile_modbus_msg_type_t msg_type)
{
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;

    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, msg_type);

    return rc;
}

/**
 * @}
 */

/** @defgroup COMMON_MODBUS_Master_Operation_Functions Common Modbus Master Operation Functions
 *  @brief    常用 modbus 主机操作函数
 @verbatim
    API 形式如下：
    - agile_modbus_serialize_xxx    打包请求数据
    返回值:
        >0:请求数据长度
        其他:异常

    - agile_modbus_deserialize_xxx  解析响应数据
    返回值:
        >=0:对应功能码响应对象的长度(如 03 功能码，值代表寄存器个数)
        其他:异常 (-1：报文错误；其他：可根据 `-128 - $返回值` 得到异常码)

 @endverbatim
 * @{
 */

int agile_modbus_serialize_read_bits(agile_modbus_t *ctx, int addr, int nb)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;

    if (nb > AGILE_MODBUS_MAX_READ_BITS)
        return -1;

    int req_length = 0;
    req_length = ctx->backend->build_request_basis(ctx, AGILE_MODBUS_FC_READ_COILS, addr, nb, ctx->send_buf);
    req_length = ctx->backend->send_msg_pre(ctx->send_buf, req_length);

    return req_length;
}

int agile_modbus_deserialize_read_bits(agile_modbus_t *ctx, int msg_length, uint8_t *dest)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;

    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_CONFIRMATION);
    if (rc < 0)
        return -1;

    rc = agile_modbus_check_confirmation(ctx, ctx->send_buf, ctx->read_buf, rc);
    if (rc < 0)
        return rc;

    int i, temp, bit;
    int pos = 0;
    int offset;
    int offset_end;
    int nb;

    offset = ctx->backend->header_length + 2;
    offset_end = offset + rc;
    nb = (ctx->send_buf[ctx->backend->header_length + 3] << 8) + ctx->send_buf[ctx->backend->header_length + 4];

    for (i = offset; i < offset_end; i++) {
        /* Shift reg hi_byte to temp */
        temp = ctx->read_buf[i];

        for (bit = 0x01; (bit & 0xff) && (pos < nb);) {
            dest[pos++] = (temp & bit) ? 1 : 0;
            bit = bit << 1;
        }
    }

    return nb;
}

int agile_modbus_serialize_read_input_bits(agile_modbus_t *ctx, int addr, int nb)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;

    if (nb > AGILE_MODBUS_MAX_READ_BITS)
        return -1;

    int req_length = 0;
    req_length = ctx->backend->build_request_basis(ctx, AGILE_MODBUS_FC_READ_DISCRETE_INPUTS, addr, nb, ctx->send_buf);
    req_length = ctx->backend->send_msg_pre(ctx->send_buf, req_length);

    return req_length;
}

int agile_modbus_deserialize_read_input_bits(agile_modbus_t *ctx, int msg_length, uint8_t *dest)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;

    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_CONFIRMATION);
    if (rc < 0)
        return -1;

    rc = agile_modbus_check_confirmation(ctx, ctx->send_buf, ctx->read_buf, rc);
    if (rc < 0)
        return rc;

    int i, temp, bit;
    int pos = 0;
    int offset;
    int offset_end;
    int nb;

    offset = ctx->backend->header_length + 2;
    offset_end = offset + rc;
    nb = (ctx->send_buf[ctx->backend->header_length + 3] << 8) + ctx->send_buf[ctx->backend->header_length + 4];

    for (i = offset; i < offset_end; i++) {
        /* Shift reg hi_byte to temp */
        temp = ctx->read_buf[i];

        for (bit = 0x01; (bit & 0xff) && (pos < nb);) {
            dest[pos++] = (temp & bit) ? 1 : 0;
            bit = bit << 1;
        }
    }

    return nb;
}

int agile_modbus_serialize_read_registers(agile_modbus_t *ctx, int addr, int nb)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;

    if (nb > AGILE_MODBUS_MAX_READ_REGISTERS)
        return -1;

    int req_length = 0;
    req_length = ctx->backend->build_request_basis(ctx, AGILE_MODBUS_FC_READ_HOLDING_REGISTERS, addr, nb, ctx->send_buf);
    req_length = ctx->backend->send_msg_pre(ctx->send_buf, req_length);

    return req_length;
}

int agile_modbus_deserialize_read_registers(agile_modbus_t *ctx, int msg_length, uint16_t *dest)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;

    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_CONFIRMATION);
    if (rc < 0)
        return -1;

    rc = agile_modbus_check_confirmation(ctx, ctx->send_buf, ctx->read_buf, rc);
    if (rc < 0)
        return rc;

    int offset;
    int i;

    offset = ctx->backend->header_length;
    for (i = 0; i < rc; i++) {
        /* shift reg hi_byte to temp OR with lo_byte */
        dest[i] = (ctx->read_buf[offset + 2 + (i << 1)] << 8) | ctx->read_buf[offset + 3 + (i << 1)];
    }

    return rc;
}

int agile_modbus_serialize_read_input_registers(agile_modbus_t *ctx, int addr, int nb)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;

    if (nb > AGILE_MODBUS_MAX_READ_REGISTERS)
        return -1;

    int req_length = 0;
    req_length = ctx->backend->build_request_basis(ctx, AGILE_MODBUS_FC_READ_INPUT_REGISTERS, addr, nb, ctx->send_buf);
    req_length = ctx->backend->send_msg_pre(ctx->send_buf, req_length);

    return req_length;
}

int agile_modbus_deserialize_read_input_registers(agile_modbus_t *ctx, int msg_length, uint16_t *dest)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;

    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_CONFIRMATION);
    if (rc < 0)
        return -1;

    rc = agile_modbus_check_confirmation(ctx, ctx->send_buf, ctx->read_buf, rc);
    if (rc < 0)
        return rc;

    int offset;
    int i;

    offset = ctx->backend->header_length;
    for (i = 0; i < rc; i++) {
        /* shift reg hi_byte to temp OR with lo_byte */
        dest[i] = (ctx->read_buf[offset + 2 + (i << 1)] << 8) | ctx->read_buf[offset + 3 + (i << 1)];
    }

    return rc;
}

int agile_modbus_serialize_write_bit(agile_modbus_t *ctx, int addr, int status)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;

    int req_length = 0;
    req_length = ctx->backend->build_request_basis(ctx, AGILE_MODBUS_FC_WRITE_SINGLE_COIL, addr, status ? 0xFF00 : 0, ctx->send_buf);
    req_length = ctx->backend->send_msg_pre(ctx->send_buf, req_length);

    return req_length;
}

int agile_modbus_deserialize_write_bit(agile_modbus_t *ctx, int msg_length)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;

    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_CONFIRMATION);
    if (rc < 0)
        return -1;

    rc = agile_modbus_check_confirmation(ctx, ctx->send_buf, ctx->read_buf, rc);

    return rc;
}

int agile_modbus_serialize_write_register(agile_modbus_t *ctx, int addr, const uint16_t value)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;

    int req_length = 0;
    req_length = ctx->backend->build_request_basis(ctx, AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER, addr, (int)value, ctx->send_buf);
    req_length = ctx->backend->send_msg_pre(ctx->send_buf, req_length);

    return req_length;
}

int agile_modbus_deserialize_write_register(agile_modbus_t *ctx, int msg_length)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;

    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_CONFIRMATION);
    if (rc < 0)
        return -1;

    rc = agile_modbus_check_confirmation(ctx, ctx->send_buf, ctx->read_buf, rc);

    return rc;
}

int agile_modbus_serialize_write_bits(agile_modbus_t *ctx, int addr, int nb, const uint8_t *src)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;

    if (nb > AGILE_MODBUS_MAX_WRITE_BITS)
        return -1;

    int i;
    int byte_count;
    int req_length;
    int bit_check = 0;
    int pos = 0;

    req_length = ctx->backend->build_request_basis(ctx, AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS, addr, nb, ctx->send_buf);
    byte_count = (nb / 8) + ((nb % 8) ? 1 : 0);

    min_req_length += (1 + byte_count);
    if (ctx->send_bufsz < min_req_length)
        return -1;

    ctx->send_buf[req_length++] = byte_count;
    for (i = 0; i < byte_count; i++) {
        int bit;

        bit = 0x01;
        ctx->send_buf[req_length] = 0;

        while ((bit & 0xFF) && (bit_check++ < nb)) {
            if (src[pos++])
                ctx->send_buf[req_length] |= bit;
            else
                ctx->send_buf[req_length] &= ~bit;

            bit = bit << 1;
        }
        req_length++;
    }

    req_length = ctx->backend->send_msg_pre(ctx->send_buf, req_length);

    return req_length;
}

int agile_modbus_deserialize_write_bits(agile_modbus_t *ctx, int msg_length)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;

    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_CONFIRMATION);
    if (rc < 0)
        return -1;

    rc = agile_modbus_check_confirmation(ctx, ctx->send_buf, ctx->read_buf, rc);

    return rc;
}

int agile_modbus_serialize_write_registers(agile_modbus_t *ctx, int addr, int nb, const uint16_t *src)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;

    if (nb > AGILE_MODBUS_MAX_WRITE_REGISTERS)
        return -1;

    int i;
    int req_length;
    int byte_count;

    req_length = ctx->backend->build_request_basis(ctx, AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS, addr, nb, ctx->send_buf);
    byte_count = nb * 2;

    min_req_length += (1 + byte_count);
    if (ctx->send_bufsz < min_req_length)
        return -1;

    ctx->send_buf[req_length++] = byte_count;
    for (i = 0; i < nb; i++) {
        ctx->send_buf[req_length++] = src[i] >> 8;
        ctx->send_buf[req_length++] = src[i] & 0x00FF;
    }

    req_length = ctx->backend->send_msg_pre(ctx->send_buf, req_length);

    return req_length;
}

int agile_modbus_deserialize_write_registers(agile_modbus_t *ctx, int msg_length)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;

    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_CONFIRMATION);
    if (rc < 0)
        return -1;

    rc = agile_modbus_check_confirmation(ctx, ctx->send_buf, ctx->read_buf, rc);

    return rc;
}

int agile_modbus_serialize_mask_write_register(agile_modbus_t *ctx, int addr, uint16_t and_mask, uint16_t or_mask)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length + 2;
    if (ctx->send_bufsz < min_req_length)
        return -1;

    int req_length = 0;
    req_length = ctx->backend->build_request_basis(ctx, AGILE_MODBUS_FC_MASK_WRITE_REGISTER, addr, 0, ctx->send_buf);

    /* HACKISH, count is not used */
    req_length -= 2;

    ctx->send_buf[req_length++] = and_mask >> 8;
    ctx->send_buf[req_length++] = and_mask & 0x00ff;
    ctx->send_buf[req_length++] = or_mask >> 8;
    ctx->send_buf[req_length++] = or_mask & 0x00ff;

    req_length = ctx->backend->send_msg_pre(ctx->send_buf, req_length);

    return req_length;
}

int agile_modbus_deserialize_mask_write_register(agile_modbus_t *ctx, int msg_length)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;

    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_CONFIRMATION);
    if (rc < 0)
        return -1;

    rc = agile_modbus_check_confirmation(ctx, ctx->send_buf, ctx->read_buf, rc);

    return rc;
}

int agile_modbus_serialize_write_and_read_registers(agile_modbus_t *ctx,
                                                    int write_addr, int write_nb,
                                                    const uint16_t *src,
                                                    int read_addr, int read_nb)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;

    if (write_nb > AGILE_MODBUS_MAX_WR_WRITE_REGISTERS)
        return -1;

    if (read_nb > AGILE_MODBUS_MAX_WR_READ_REGISTERS)
        return -1;

    int req_length;
    int i;
    int byte_count;

    req_length = ctx->backend->build_request_basis(ctx, AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS, read_addr, read_nb, ctx->send_buf);
    byte_count = write_nb * 2;

    min_req_length += (5 + byte_count);
    if (ctx->send_bufsz < min_req_length)
        return -1;

    ctx->send_buf[req_length++] = write_addr >> 8;
    ctx->send_buf[req_length++] = write_addr & 0x00ff;
    ctx->send_buf[req_length++] = write_nb >> 8;
    ctx->send_buf[req_length++] = write_nb & 0x00ff;
    ctx->send_buf[req_length++] = byte_count;
    for (i = 0; i < write_nb; i++) {
        ctx->send_buf[req_length++] = src[i] >> 8;
        ctx->send_buf[req_length++] = src[i] & 0x00FF;
    }

    req_length = ctx->backend->send_msg_pre(ctx->send_buf, req_length);

    return req_length;
}

int agile_modbus_deserialize_write_and_read_registers(agile_modbus_t *ctx, int msg_length, uint16_t *dest)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;

    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_CONFIRMATION);
    if (rc < 0)
        return -1;

    rc = agile_modbus_check_confirmation(ctx, ctx->send_buf, ctx->read_buf, rc);
    if (rc < 0)
        return rc;

    int offset;
    int i;

    offset = ctx->backend->header_length;
    for (i = 0; i < rc; i++) {
        /* shift reg hi_byte to temp OR with lo_byte */
        dest[i] = (ctx->read_buf[offset + 2 + (i << 1)] << 8) | ctx->read_buf[offset + 3 + (i << 1)];
    }

    return rc;
}

int agile_modbus_serialize_report_slave_id(agile_modbus_t *ctx)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;

    int req_length = 0;
    req_length = ctx->backend->build_request_basis(ctx, AGILE_MODBUS_FC_REPORT_SLAVE_ID, 0, 0, ctx->send_buf);
    /* HACKISH, addr and count are not used */
    req_length -= 4;
    req_length = ctx->backend->send_msg_pre(ctx->send_buf, req_length);

    return req_length;
}

int agile_modbus_deserialize_report_slave_id(agile_modbus_t *ctx, int msg_length, int max_dest, uint8_t *dest)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;
    if (max_dest <= 0)
        return -1;

    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_CONFIRMATION);
    if (rc < 0)
        return -1;

    rc = agile_modbus_check_confirmation(ctx, ctx->send_buf, ctx->read_buf, rc);
    if (rc < 0)
        return rc;

    int i;
    int offset;

    offset = ctx->backend->header_length + 2;

    /* Byte count, slave id, run indicator status and
       additional data. Truncate copy to max_dest. */
    for (i = 0; i < rc && i < max_dest; i++) {
        dest[i] = ctx->read_buf[offset + i];
    }

    return rc;
}

/**
 * @}
 */

/** @defgroup RAW_MODBUS_Master_Operation_Functions RAW Modbus Master Operation Functions
 * @{
 */

/**
 * @brief   将原始数据打包成请求报文
 * @param   ctx modbus 句柄
 * @param   raw_req 原始报文(PDU + Slave address)
 * @param   raw_req_length 原始报文长度
 * @return  >0:请求数据长度; 其他:异常
 */
int agile_modbus_serialize_raw_request(agile_modbus_t *ctx, const uint8_t *raw_req, int raw_req_length)
{
    if ((raw_req_length < 2) || (raw_req_length > (AGILE_MODBUS_MAX_PDU_LENGTH + 1))) {
        /* The raw request must contain function and slave at least and
           must not be longer than the maximum pdu length plus the slave
           address. */

        return -1;
    }

    int min_req_length = ctx->backend->header_length + 1 + ctx->backend->checksum_length + raw_req_length - 2;
    if (ctx->send_bufsz < min_req_length)
        return -1;

    agile_modbus_sft_t sft;
    int req_length;

    sft.slave = raw_req[0];
    sft.function = raw_req[1];
    /* The t_id is left to zero */
    sft.t_id = 0;
    /* This response function only set the header so it's convenient here */
    req_length = ctx->backend->build_response_basis(&sft, ctx->send_buf);

    if (raw_req_length > 2) {
        /* Copy data after function code */
        memcpy(ctx->send_buf + req_length, raw_req + 2, raw_req_length - 2);
        req_length += raw_req_length - 2;
    }

    req_length = ctx->backend->send_msg_pre(ctx->send_buf, req_length);

    return req_length;
}

/**
 * @brief   解析响应原始数据
 * @param   ctx modbus 句柄
 * @param   msg_length 接收数据长度
 * @return  >=0:对应功能码响应对象的长度(如 03 功能码，值代表寄存器个数);
 *          其他:异常 (-1：报文错误；其他：可根据 `-128 - $返回值` 得到异常码)
 */
int agile_modbus_deserialize_raw_response(agile_modbus_t *ctx, int msg_length)
{
    int min_req_length = ctx->backend->header_length + 5 + ctx->backend->checksum_length;
    if (ctx->send_bufsz < min_req_length)
        return -1;
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;

    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_CONFIRMATION);
    if (rc < 0)
        return -1;

    rc = agile_modbus_check_confirmation(ctx, ctx->send_buf, ctx->read_buf, rc);

    return rc;
}

/**
 * @}
 */

/** @defgroup COMMON_MODBUS_Slave_Operation_Functions Common Modbus Slave Operation Functions
 * @{
 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
