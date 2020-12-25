#include "agile_modbus.h"
#include <string.h>

#define AGILE_MODBUS_MSG_LENGTH_UNDEFINED       -1


void agile_modbus_common_init(agile_modbus_t *ctx, uint8_t *send_buf, int send_bufsz, uint8_t *read_buf, int read_bufsz)
{
    memset(ctx, 0, sizeof(agile_modbus_t));
    ctx->slave = -1;
    ctx->send_buf = send_buf;
    ctx->send_bufsz = send_bufsz;
    ctx->read_buf = read_buf;
    ctx->read_bufsz = read_bufsz;
}

/* Define the slave number */
int agile_modbus_set_slave(agile_modbus_t *ctx, int slave)
{
    return ctx->backend->set_slave(ctx, slave);
}

/*
 *  ---------- Request     Indication ----------
 *  | Client | ---------------------->| Server |
 *  ---------- Confirmation  Response ----------
 */

/* Computes the length to read after the function received */
static uint8_t agile_modbus_compute_meta_length_after_function(int function, agile_modbus_msg_type_t msg_type)
{
    int length;

    if (msg_type == AGILE_MODBUS_MSG_INDICATION)
    {
        if (function <= AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER)
        {
            length = 4;
        }
        else if (function == AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS ||
                 function == AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS)
        {
            length = 5;
        }
        else if (function == AGILE_MODBUS_FC_MASK_WRITE_REGISTER)
        {
            length = 6;
        }
        else if (function == AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS)
        {
            length = 9;
        }
        else
        {
            /* MODBUS_FC_READ_EXCEPTION_STATUS, MODBUS_FC_REPORT_SLAVE_ID */
            length = 0;
        }
    }
    else
    {
        /* MSG_CONFIRMATION */
        switch (function)
        {
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
                length = 0;
        }
    }

    return length;
}

/* Computes the length to read after the meta information (address, count, etc) */
static int agile_modbus_compute_data_length_after_meta(agile_modbus_t *ctx, uint8_t *msg, agile_modbus_msg_type_t msg_type)
{
    int function = msg[ctx->backend->header_length];
    int length;

    if (msg_type == AGILE_MODBUS_MSG_INDICATION)
    {
        switch (function)
        {
            case AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS:
            case AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
                length = msg[ctx->backend->header_length + 5];
            break;

            case AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS:
                length = msg[ctx->backend->header_length + 9];
            break;

            default:
                length = 0;
        }
    }
    else
    {
        /* MSG_CONFIRMATION */
        if (function <= AGILE_MODBUS_FC_READ_INPUT_REGISTERS ||
            function == AGILE_MODBUS_FC_REPORT_SLAVE_ID ||
            function == AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS)
        {
            length = msg[ctx->backend->header_length + 1];
        }
        else
        {
            length = 0;
        }
    }

    length += ctx->backend->checksum_length;

    return length;
}

static int agile_modbus_receive_msg_judge(agile_modbus_t *ctx, uint8_t *msg, int msg_length, agile_modbus_msg_type_t msg_type)
{
    int remain_len = msg_length;

    if (remain_len > (int)ctx->backend->max_adu_length)
        return -1;
    remain_len -= (ctx->backend->header_length + 1);
    if (remain_len < 0)
        return -1;
    remain_len -= agile_modbus_compute_meta_length_after_function(msg[ctx->backend->header_length], msg_type);
    if (remain_len < 0)
        return -1;
    remain_len -= agile_modbus_compute_data_length_after_meta(ctx, msg, msg_type);
    if (remain_len < 0)
        return -1;
    
    return ctx->backend->check_integrity(ctx, msg, msg_length);
}

/* Computes the length of the expected response */
static int agile_modbus_compute_response_length_from_request(agile_modbus_t *ctx, uint8_t *req)
{
    int length;
    const int offset = ctx->backend->header_length;

    switch (req[offset])
    {
        case AGILE_MODBUS_FC_READ_COILS:
        case AGILE_MODBUS_FC_READ_DISCRETE_INPUTS:
        {
            /* Header + nb values (code from write_bits) */
            int nb = (req[offset + 3] << 8) | req[offset + 4];
            length = 2 + (nb / 8) + ((nb % 8) ? 1 : 0);
        }
        break;

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

static int agile_modbus_check_confirmation(agile_modbus_t *ctx, uint8_t *req,
                                           uint8_t *rsp, int rsp_length)
{
    int rc;
    int rsp_length_computed;
    const int offset = ctx->backend->header_length;
    const int function = rsp[offset];

    if (ctx->backend->pre_check_confirmation)
    {
        rc = ctx->backend->pre_check_confirmation(ctx, req, rsp, rsp_length);
        if (rc < 0)
            return -1;
    }

    rsp_length_computed = agile_modbus_compute_response_length_from_request(ctx, req);

    /* Exception code */
    if (function >= 0x80)
        return -1;

    /* Check length */
    if ((rsp_length == rsp_length_computed || rsp_length_computed == AGILE_MODBUS_MSG_LENGTH_UNDEFINED) && function < 0x80)
    {
        int req_nb_value;
        int rsp_nb_value;

        /* Check function code */
        if (function != req[offset])
            return -1;

        /* Check the number of values is corresponding to the request */
        switch (function)
        {
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
    }
    else
        rc = -1;

    return rc;
}

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
        return -1;

    int i, temp, bit;
    int pos = 0;
    int offset;
    int offset_end;
    int nb;

    offset = ctx->backend->header_length + 2;
    offset_end = offset + rc;
    nb = (ctx->send_buf[ctx->backend->header_length + 3] << 8) + ctx->send_buf[ctx->backend->header_length + 4];

    for (i = offset; i < offset_end; i++)
    {
        /* Shift reg hi_byte to temp */
        temp = ctx->read_buf[i];

        for (bit = 0x01; (bit & 0xff) && (pos < nb);)
        {
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
        return -1;

    int i, temp, bit;
    int pos = 0;
    int offset;
    int offset_end;
    int nb;

    offset = ctx->backend->header_length + 2;
    offset_end = offset + rc;
    nb = (ctx->send_buf[ctx->backend->header_length + 3] << 8) + ctx->send_buf[ctx->backend->header_length + 4];

    for (i = offset; i < offset_end; i++)
    {
        /* Shift reg hi_byte to temp */
        temp = ctx->read_buf[i];

        for (bit = 0x01; (bit & 0xff) && (pos < nb);)
        {
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
        return -1;
    
    int offset;
    int i;

    offset = ctx->backend->header_length;
    for (i = 0; i < rc; i++)
    {
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
        return -1;
    
    int offset;
    int i;

    offset = ctx->backend->header_length;
    for (i = 0; i < rc; i++)
    {
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
    req_length = ctx->backend->build_request_basis(ctx, AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER, addr, (int) value, ctx->send_buf);
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
    for (i = 0; i < byte_count; i++)
    {
        int bit;

        bit = 0x01;
        ctx->send_buf[req_length] = 0;

        while ((bit & 0xFF) && (bit_check++ < nb))
        {
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
    for (i = 0; i < nb; i++)
    {
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
    for (i = 0; i < write_nb; i++)
    {
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
        return -1;
    
    int offset;
    int i;

    offset = ctx->backend->header_length;
    for (i = 0; i < rc; i++)
    {
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
    if(max_dest <= 0)
        return -1;
    
    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_CONFIRMATION);
    if (rc < 0)
        return -1;
    
    rc = agile_modbus_check_confirmation(ctx, ctx->send_buf, ctx->read_buf, rc);
    if (rc < 0)
        return -1;
    
    int i;
    int offset;

    offset = ctx->backend->header_length + 2;

    /* Byte count, slave id, run indicator status and
       additional data. Truncate copy to max_dest. */
    for (i = 0; i < rc && i < max_dest; i++)
    {
        dest[i] = ctx->read_buf[offset + i];
    }

    return rc;
}

int agile_modbus_serialize_raw_request(agile_modbus_t *ctx, const uint8_t *raw_req, int raw_req_length)
{
    if ((raw_req_length < 2) || (raw_req_length > (AGILE_MODBUS_MAX_PDU_LENGTH + 1)))
    {
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

    if (raw_req_length > 2)
    {
        /* Copy data after function code */
        memcpy(ctx->send_buf + req_length, raw_req + 2, raw_req_length - 2);
        req_length += raw_req_length - 2;
    }

    req_length = ctx->backend->send_msg_pre(ctx->send_buf, req_length);

    return req_length;
}

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

int agile_modbus_receive_judge(agile_modbus_t *ctx, int msg_length)
{
    if ((msg_length <= 0) || (msg_length > ctx->read_bufsz))
        return -1;
    
    int rc = agile_modbus_receive_msg_judge(ctx, ctx->read_buf, msg_length, AGILE_MODBUS_MSG_INDICATION);

    return rc;
}
