#ifndef __PKG_AGILE_MODBUS_H
#define __PKG_AGILE_MODBUS_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Modbus function codes */
#define AGILE_MODBUS_FC_READ_COILS                0x01
#define AGILE_MODBUS_FC_READ_DISCRETE_INPUTS      0x02
#define AGILE_MODBUS_FC_READ_HOLDING_REGISTERS    0x03
#define AGILE_MODBUS_FC_READ_INPUT_REGISTERS      0x04
#define AGILE_MODBUS_FC_WRITE_SINGLE_COIL         0x05
#define AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER     0x06
#define AGILE_MODBUS_FC_READ_EXCEPTION_STATUS     0x07
#define AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS      0x0F
#define AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS  0x10
#define AGILE_MODBUS_FC_REPORT_SLAVE_ID           0x11
#define AGILE_MODBUS_FC_MASK_WRITE_REGISTER       0x16
#define AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS  0x17

#define AGILE_MODBUS_BROADCAST_ADDRESS    0

/* Modbus_Application_Protocol_V1_1b.pdf (chapter 6 section 1 page 12)
 * Quantity of Coils to read (2 bytes): 1 to 2000 (0x7D0)
 * (chapter 6 section 11 page 29)
 * Quantity of Coils to write (2 bytes): 1 to 1968 (0x7B0)
 */
#define AGILE_MODBUS_MAX_READ_BITS              2000
#define AGILE_MODBUS_MAX_WRITE_BITS             1968

/* Modbus_Application_Protocol_V1_1b.pdf (chapter 6 section 3 page 15)
 * Quantity of Registers to read (2 bytes): 1 to 125 (0x7D)
 * (chapter 6 section 12 page 31)
 * Quantity of Registers to write (2 bytes) 1 to 123 (0x7B)
 * (chapter 6 section 17 page 38)
 * Quantity of Registers to write in R/W registers (2 bytes) 1 to 121 (0x79)
 */
#define AGILE_MODBUS_MAX_READ_REGISTERS          125
#define AGILE_MODBUS_MAX_WRITE_REGISTERS         123
#define AGILE_MODBUS_MAX_WR_WRITE_REGISTERS      121
#define AGILE_MODBUS_MAX_WR_READ_REGISTERS       125

/* The size of the MODBUS PDU is limited by the size constraint inherited from
 * the first MODBUS implementation on Serial Line network (max. RS485 ADU = 256
 * bytes). Therefore, MODBUS PDU for serial line communication = 256 - Server
 * address (1 byte) - CRC (2 bytes) = 253 bytes.
 */
#define AGILE_MODBUS_MAX_PDU_LENGTH              253

/* Consequently:
 * - RTU MODBUS ADU = 253 bytes + Server address (1 byte) + CRC (2 bytes) = 256
 *   bytes.
 * - TCP MODBUS ADU = 253 bytes + MBAP (7 bytes) = 260 bytes.
 * so the maximum of both backend in 260 bytes. This size can used to allocate
 * an array of bytes to store responses and it will be compatible with the two
 * backends.
 */
#define AGILE_MODBUS_MAX_ADU_LENGTH              260


typedef enum
{
    AGILE_MODBUS_BACKEND_TYPE_RTU = 0,
    AGILE_MODBUS_BACKEND_TYPE_TCP
} agile_modbus_backend_type_t;

/*
 *  ---------- Request     Indication ----------
 *  | Client | ---------------------->| Server |
 *  ---------- Confirmation  Response ----------
 */
typedef enum
{
    /* Request message on the server side */
    AGILE_MODBUS_MSG_INDICATION,
    /* Request message on the client side */
    AGILE_MODBUS_MSG_CONFIRMATION
} agile_modbus_msg_type_t;

typedef struct agile_modbus_sft
{
    int slave;
    int function;
    int t_id;
} agile_modbus_sft_t;

typedef struct agile_modbus agile_modbus_t;

typedef struct agile_modbus_backend
{
    uint32_t backend_type;
    uint32_t header_length;
    uint32_t checksum_length;
    uint32_t max_adu_length;
    int (*set_slave) (agile_modbus_t *ctx, int slave);
    int (*build_request_basis) (agile_modbus_t *ctx, int function, int addr,
                                int nb, uint8_t *req);
    int (*build_response_basis) (agile_modbus_sft_t *sft, uint8_t *rsp);
    int (*prepare_response_tid) (const uint8_t *req, int *req_length);
    int (*send_msg_pre) (uint8_t *req, int req_length);
    int (*check_integrity) (agile_modbus_t *ctx, uint8_t *msg, const int msg_length);
    int (*pre_check_confirmation) (agile_modbus_t *ctx, const uint8_t *req,
                                   const uint8_t *rsp, int rsp_length);
} agile_modbus_backend_t;

struct agile_modbus
{
    int slave;
    uint8_t *send_buf;
    int send_bufsz;
    uint8_t *read_buf;
    int read_bufsz;
    const agile_modbus_backend_t *backend;
    void *backend_data;
};

void agile_modbus_common_init(agile_modbus_t *ctx, uint8_t *send_buf, int send_bufsz, uint8_t *read_buf, int read_bufsz);
int agile_modbus_set_slave(agile_modbus_t *ctx, int slave);

int agile_modbus_serialize_read_bits(agile_modbus_t *ctx, int addr, int nb);
int agile_modbus_deserialize_read_bits(agile_modbus_t *ctx, int msg_length, uint8_t *dest);
int agile_modbus_serialize_read_input_bits(agile_modbus_t *ctx, int addr, int nb);
int agile_modbus_deserialize_read_input_bits(agile_modbus_t *ctx, int msg_length, uint8_t *dest);
int agile_modbus_serialize_read_registers(agile_modbus_t *ctx, int addr, int nb);
int agile_modbus_deserialize_read_registers(agile_modbus_t *ctx, int msg_length, uint16_t *dest);
int agile_modbus_serialize_read_input_registers(agile_modbus_t *ctx, int addr, int nb);
int agile_modbus_deserialize_read_input_registers(agile_modbus_t *ctx, int msg_length, uint16_t *dest);
int agile_modbus_serialize_write_bit(agile_modbus_t *ctx, int addr, int status);
int agile_modbus_deserialize_write_bit(agile_modbus_t *ctx, int msg_length);
int agile_modbus_serialize_write_register(agile_modbus_t *ctx, int addr, const uint16_t value);
int agile_modbus_deserialize_write_register(agile_modbus_t *ctx, int msg_length);
int agile_modbus_serialize_write_bits(agile_modbus_t *ctx, int addr, int nb, const uint8_t *src);
int agile_modbus_deserialize_write_bits(agile_modbus_t *ctx, int msg_length);
int agile_modbus_serialize_write_registers(agile_modbus_t *ctx, int addr, int nb, const uint16_t *src);
int agile_modbus_deserialize_write_registers(agile_modbus_t *ctx, int msg_length);
int agile_modbus_serialize_mask_write_register(agile_modbus_t *ctx, int addr, uint16_t and_mask, uint16_t or_mask);
int agile_modbus_deserialize_mask_write_register(agile_modbus_t *ctx, int msg_length);
int agile_modbus_serialize_write_and_read_registers(agile_modbus_t *ctx,
                                                    int write_addr, int write_nb,
                                                    const uint16_t *src,
                                                    int read_addr, int read_nb);
int agile_modbus_deserialize_write_and_read_registers(agile_modbus_t *ctx, int msg_length, uint16_t *dest);
int agile_modbus_serialize_report_slave_id(agile_modbus_t *ctx);
int agile_modbus_deserialize_report_slave_id(agile_modbus_t *ctx, int msg_length, int max_dest, uint8_t *dest);
int agile_modbus_serialize_raw_request(agile_modbus_t *ctx, const uint8_t *raw_req, int raw_req_length);
int agile_modbus_deserialize_raw_response(agile_modbus_t *ctx, int msg_length);
int agile_modbus_receive_judge(agile_modbus_t *ctx, int msg_length);

#include "agile_modbus_rtu.h"
#include "agile_modbus_tcp.h"

#ifdef __cplusplus
}
#endif

#endif
