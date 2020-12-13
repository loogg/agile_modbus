#ifndef __PKG_AGILE_MODBUS_TCP_H
#define __PKG_AGILE_MODBUS_TCP_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AGILE_MODBUS_TCP_HEADER_LENGTH      7
#define AGILE_MODBUS_TCP_PRESET_REQ_LENGTH  12
#define AGILE_MODBUS_TCP_PRESET_RSP_LENGTH  8

#define AGILE_MODBUS_TCP_CHECKSUM_LENGTH    0

/* Modbus_Application_Protocol_V1_1b.pdf Chapter 4 Section 1 Page 5
 * TCP MODBUS ADU = 253 bytes + MBAP (7 bytes) = 260 bytes
 */
#define AGILE_MODBUS_TCP_MAX_ADU_LENGTH     260

typedef struct agile_modbus_tcp
{
    agile_modbus_t _ctx;
    /* Extract from MODBUS Messaging on TCP/IP Implementation Guide V1.0b
       (page 23/46):
       The transaction identifier is used to associate the future response
       with the request. This identifier is unique on each TCP connection. */
    uint16_t t_id;
} agile_modbus_tcp_t;

int agile_modbus_tcp_init(agile_modbus_tcp_t *ctx, uint8_t *send_buf, int send_bufsz, uint8_t *read_buf, int read_bufsz);

#ifdef __cplusplus
}
#endif

#endif
