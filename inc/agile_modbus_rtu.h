#ifndef __PKG_AGILE_MODBUS_RTU_H
#define __PKG_AGILE_MODBUS_RTU_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AGILE_MODBUS_RTU_HEADER_LENGTH      1
#define AGILE_MODBUS_RTU_PRESET_REQ_LENGTH  6
#define AGILE_MODBUS_RTU_PRESET_RSP_LENGTH  2

#define AGILE_MODBUS_RTU_CHECKSUM_LENGTH    2

/* Modbus_Application_Protocol_V1_1b.pdf Chapter 4 Section 1 Page 5
 * RS232 / RS485 ADU = 253 bytes + slave (1 byte) + CRC (2 bytes) = 256 bytes
 */
#define AGILE_MODBUS_RTU_MAX_ADU_LENGTH     256


typedef struct agile_modbus_rtu
{
    agile_modbus_t _ctx;
} agile_modbus_rtu_t;

int agile_modbus_rtu_init(agile_modbus_rtu_t *ctx, uint8_t *send_buf, int send_bufsz, uint8_t *read_buf, int read_bufsz);

#ifdef __cplusplus
}
#endif

#endif
