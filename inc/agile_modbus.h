/**
 * @file    agile_modbus.h
 * @brief   Agile Modbus software package common header file
 * @author  Ma Longwei (2544047213@qq.com)
 * @date    2022-07-28
 *
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 Ma Longwei.
 * All rights reserved.</center></h2>
 *
 */

#ifndef __PKG_AGILE_MODBUS_H
#define __PKG_AGILE_MODBUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** @addtogroup COMMON
 * @{
 */

/** @defgroup Modbus_Module_Definition Modbus Module Definition
 * @{
 */
#ifndef AGILE_MODBUS_USING_RTU
#define AGILE_MODBUS_USING_RTU 1
#endif /* AGILE_MODBUS_USING_RTU */

#ifndef AGILE_MODBUS_USING_TCP
#define AGILE_MODBUS_USING_TCP 1
#endif /* AGILE_MODBUS_USING_TCP */
/**
 * @}
 */

/** @defgroup COMMON_Exported_Constants Common Exported Constants
 * @{
 */

/** @defgroup Modbus_Function_Codes Modbus Function Codes
 * @{
 */
#define AGILE_MODBUS_FC_READ_COILS               0x01
#define AGILE_MODBUS_FC_READ_DISCRETE_INPUTS     0x02
#define AGILE_MODBUS_FC_READ_HOLDING_REGISTERS   0x03
#define AGILE_MODBUS_FC_READ_INPUT_REGISTERS     0x04
#define AGILE_MODBUS_FC_WRITE_SINGLE_COIL        0x05
#define AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER    0x06
#define AGILE_MODBUS_FC_READ_EXCEPTION_STATUS    0x07
#define AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS     0x0F
#define AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS 0x10
#define AGILE_MODBUS_FC_REPORT_SLAVE_ID          0x11
#define AGILE_MODBUS_FC_MASK_WRITE_REGISTER      0x16
#define AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS 0x17
/**
 * @}
 */

/** @defgroup Modbus_Constants Modbus Constants
 * @{
 */
#define AGILE_MODBUS_VERSION_STRING "AMB_1.1.4" /**< Agile Modbus version number */

#define AGILE_MODBUS_BROADCAST_ADDRESS 0 /**< Modbus broadcast address */

/** @name Quantity limit of Coils
 @verbatim
    Modbus_Application_Protocol_V1_1b.pdf (chapter 6 section 1 page 12)
    Quantity of Coils to read (2 bytes): 1 to 2000 (0x7D0)
    (chapter 6 section 11 page 29)
    Quantity of Coils to write (2 bytes): 1 to 1968 (0x7B0)

 @endverbatim
 * @{
 */
#define AGILE_MODBUS_MAX_READ_BITS  2000
#define AGILE_MODBUS_MAX_WRITE_BITS 1968
/**
 * @}
 */

/** @name Quantity limit of Registers
 @verbatim
    Modbus_Application_Protocol_V1_1b.pdf (chapter 6 section 3 page 15)
    Quantity of Registers to read (2 bytes): 1 to 125 (0x7D)
    (chapter 6 section 12 page 31)
    Quantity of Registers to write (2 bytes) 1 to 123 (0x7B)
    (chapter 6 section 17 page 38)
    Quantity of Registers to write in R/W registers (2 bytes) 1 to 121 (0x79)

 @endverbatim
 * @{
 */
#define AGILE_MODBUS_MAX_READ_REGISTERS     125
#define AGILE_MODBUS_MAX_WRITE_REGISTERS    123
#define AGILE_MODBUS_MAX_WR_WRITE_REGISTERS 121
#define AGILE_MODBUS_MAX_WR_READ_REGISTERS  125
/**
 * @}
 */

/**
 @verbatim
    The size of the MODBUS PDU is limited by the size constraint inherited from
    the first MODBUS implementation on Serial Line network (max. RS485 ADU = 256
    bytes). Therefore, MODBUS PDU for serial line communication = 256 - Server
    address (1 byte) - CRC (2 bytes) = 253 bytes.

 @endverbatim
 */
#define AGILE_MODBUS_MAX_PDU_LENGTH 253

/**
 @verbatim
    Consequently:
    - RTU MODBUS ADU = 253 bytes + Server address (1 byte) + CRC (2 bytes) = 256
    bytes.
    - TCP MODBUS ADU = 253 bytes + MBAP (7 bytes) = 260 bytes.
    so the maximum of both backend in 260 bytes. This size can used to allocate
    an array of bytes to store responses and it will be compatible with the two
    backends.

 @endverbatim
 */
#define AGILE_MODBUS_MAX_ADU_LENGTH 260
/**
 * @}
 */

/**
 * @}
 */

/** @defgroup COMMON_Exported_Types Common Exported Types
 * @{
 */

/**
 * @brief   Modbus exception code
 */
enum {
    AGILE_MODBUS_EXCEPTION_ILLEGAL_FUNCTION = 0x01,
    AGILE_MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS,
    AGILE_MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE,
    AGILE_MODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE,
    AGILE_MODBUS_EXCEPTION_ACKNOWLEDGE,
    AGILE_MODBUS_EXCEPTION_SLAVE_OR_SERVER_BUSY,
    AGILE_MODBUS_EXCEPTION_NEGATIVE_ACKNOWLEDGE,
    AGILE_MODBUS_EXCEPTION_MEMORY_PARITY,
    AGILE_MODBUS_EXCEPTION_NOT_DEFINED,
    AGILE_MODBUS_EXCEPTION_GATEWAY_PATH,
    AGILE_MODBUS_EXCEPTION_GATEWAY_TARGET,
    AGILE_MODBUS_EXCEPTION_UNKNOW = 0xff
};

/**
 *@ brief   Modbus backend type
 */
typedef enum {
    AGILE_MODBUS_BACKEND_TYPE_RTU = 0, /**< RTU */
    AGILE_MODBUS_BACKEND_TYPE_TCP      /**< TCP */
} agile_modbus_backend_type_t;

/**
 * @brief   Modbus received message type
 *
 @verbatim
    ---------- Request     Indication ----------
    | Client | ---------------------->| Server |
    ---------- Confirmation  Response ----------

 @endverbatim
 */
typedef enum {
    AGILE_MODBUS_MSG_INDICATION,  /**< Host-side request message */
    AGILE_MODBUS_MSG_CONFIRMATION /**< Server-side request message */
} agile_modbus_msg_type_t;

/**
 * @brief   contains the modbus header parameter structure
 */
typedef struct agile_modbus_sft {
    int slave;    /**< slave address */
    int function; /**< function code */
    int t_id;     /**< Transaction identifier */
} agile_modbus_sft_t;

typedef struct agile_modbus agile_modbus_t; /**< Agile Modbus structure */

/**
 * @brief   Agile Modbus backend interface structure
 */
typedef struct agile_modbus_backend {
    uint32_t backend_type;                            /**< Backend type */
    uint32_t header_length;                           /**<Header length, excluding function code */
    uint32_t checksum_length;                         /**< Check data length */
    uint32_t max_adu_length;                          /**< Backend ADU length */
    int (*set_slave)(agile_modbus_t *ctx, int slave); /**< Set address interface */
    int (*build_request_basis)(agile_modbus_t *ctx, int function, int addr,
                               int nb, uint8_t *req);                                /**< Build a basic request message interface */
    int (*build_response_basis)(agile_modbus_sft_t *sft, uint8_t *rsp);              /**< Build a basic response message interface */
    int (*prepare_response_tid)(const uint8_t *req, int *req_length);                /**< Prepare response interface */
    int (*send_msg_pre)(uint8_t *req, int req_length);                               /**< Pre-send data interface */
    int (*check_integrity)(agile_modbus_t *ctx, uint8_t *msg, const int msg_length); /**< Check the receive data integrity interface */
    int (*pre_check_confirmation)(agile_modbus_t *ctx, const uint8_t *req,
                                  const uint8_t *rsp, int rsp_length); /**< Pre-check confirmation interface */
} agile_modbus_backend_t;

/**
 * @brief   Agile Modbus structure
 */
struct agile_modbus {
    int slave;         /**< slave address */
    uint8_t *send_buf; /**< Send buffer */
    int send_bufsz;    /**<Send buffer size */
    uint8_t *read_buf; /**<Receive buffer */
    int read_bufsz;    /**<Receive buffer size */
    uint8_t (*compute_meta_length_after_function)(agile_modbus_t *ctx, int function,
                                                  agile_modbus_msg_type_t msg_type); /**< Customized calculation data element length interface */
    int (*compute_data_length_after_meta)(agile_modbus_t *ctx, uint8_t *msg,
                                          int msg_length, agile_modbus_msg_type_t msg_type); /**< Customized calculation data length interface */
    const agile_modbus_backend_t *backend;                                                   /**< Backend interface */
    void *backend_data;                                                                      /**< Backend data, pointing to RTU or TCP structure */
};

/**
 * @}
 */

/** @addtogroup Modbus_Slave
 * @{
 */

/** @defgroup Slave_Exported_Types Slave Exported Types
 * @{
 */

/**
 * @brief   Agile Modbus slave information structure
 */
struct agile_modbus_slave_info {
    agile_modbus_sft_t *sft; /**< sft structure pointer */
    int *rsp_length;         /**<Response data length pointer */
    int address;             /**< Register address */
    int nb;                  /**< number */
    uint8_t *buf;            /**< Data fields required for different function codes */
    int send_index;          /**< Current index of sending buffer */
};

/**
 * @brief   Slave callback function
 * @param   ctx modbus handle
 * @param   slave_info slave information body
 * @param   data private data
 * @return  =0: normal;
 *          <0: Abnormal
 *             (-AGILE_MODBUS_EXCEPTION_UNKNOW(-255): Unknown exception, the slave will not package the response data)
 *             (Other negative exception codes: package exception response data from the opportunity)
 */
typedef int (*agile_modbus_slave_callback_t)(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, const void *data);

/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup COMMON_Exported_Functions
 * @{
 */
void agile_modbus_common_init(agile_modbus_t *ctx, uint8_t *send_buf, int send_bufsz, uint8_t *read_buf, int read_bufsz);
int agile_modbus_set_slave(agile_modbus_t *ctx, int slave);
void agile_modbus_set_compute_meta_length_after_function_cb(agile_modbus_t *ctx,
                                                            uint8_t (*cb)(agile_modbus_t *ctx, int function,
                                                                          agile_modbus_msg_type_t msg_type));
void agile_modbus_set_compute_data_length_after_meta_cb(agile_modbus_t *ctx,
                                                        int (*cb)(agile_modbus_t *ctx, uint8_t *msg,
                                                                  int msg_length, agile_modbus_msg_type_t msg_type));
int agile_modbus_receive_judge(agile_modbus_t *ctx, int msg_length, agile_modbus_msg_type_t msg_type);
/**
 * @}
 */

/** @addtogroup Modbus_Master
 * @{
 */

/** @addtogroup Master_Common_Operation_Functions
 * @{
 */
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
/**
 * @}
 */

/** @addtogroup Master_Raw_Operation_Functions
 * @{
 */
int agile_modbus_compute_response_length_from_request(agile_modbus_t *ctx, uint8_t *req);
int agile_modbus_serialize_raw_request(agile_modbus_t *ctx, const uint8_t *raw_req, int raw_req_length);
int agile_modbus_deserialize_raw_response(agile_modbus_t *ctx, int msg_length);
/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup Modbus_Slave
 * @{
 */

/** @addtogroup Slave_Operation_Functions
 * @{
 */
int agile_modbus_slave_handle(agile_modbus_t *ctx, int msg_length, uint8_t slave_strict,
                              agile_modbus_slave_callback_t slave_cb, const void *slave_data, int *frame_length);
void agile_modbus_slave_io_set(uint8_t *buf, int index, int status);
uint8_t agile_modbus_slave_io_get(uint8_t *buf, int index);
void agile_modbus_slave_register_set(uint8_t *buf, int index, uint16_t data);
uint16_t agile_modbus_slave_register_get(uint8_t *buf, int index);
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/* Include RTU and TCP module */
#if AGILE_MODBUS_USING_RTU
#include "agile_modbus_rtu.h"
#endif /* AGILE_MODBUS_USING_RTU */

#if AGILE_MODBUS_USING_TCP
#include "agile_modbus_tcp.h"
#endif /* AGILE_MODBUS_USING_TCP */

#ifdef __cplusplus
}
#endif

#endif /* __PKG_AGILE_MODBUS_H */
