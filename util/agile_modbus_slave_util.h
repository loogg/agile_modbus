/**
 * @file    agile_modbus_slave_util.h
 * @brief   The simple slave access header file provided by the Agile Modbus software package
 * @author  Ma Longwei (2544047213@qq.com)
 * @date    2022-07-28
 *
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2022 Ma Longwei.
 * All rights reserved.</center></h2>
 *
 */

#ifndef __PKG_AGILE_MODBUS_SLAVE_UTIL_H
#define __PKG_AGILE_MODBUS_SLAVE_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** @addtogroup UTIL
 * @{
 */

/** @addtogroup SLAVE_UTIL
 * @{
 */

/** @defgroup SLAVE_UTIL_Exported_Types Slave Util Exported Types
 * @{
 */

/**
 * @brief   slave register mapping structure
 */
typedef struct agile_modbus_slave_util_map {
    int start_addr;                                       /**<Start address */
    int end_addr;                                         /**< end address */
    int (*get)(void *buf, int bufsz);                     /**< Get register data interface */
    int (*set)(int index, int len, void *buf, int bufsz); /**< Set register data interface */
} agile_modbus_slave_util_map_t;

/**
 * @brief   slave function structure
 */
typedef struct agile_modbus_slave_util {
    const agile_modbus_slave_util_map_t *tab_bits;                                            /**< Coil register definition array */
    int nb_bits;                                                                              /**<The number of coil register definition arrays */
    const agile_modbus_slave_util_map_t *tab_input_bits;                                      /**< Discrete input register definition array */
    int nb_input_bits;                                                                        /**<The number of discrete input register definition arrays */
    const agile_modbus_slave_util_map_t *tab_registers;                                       /**< Holding register definition array */
    int nb_registers;                                                                         /**< Number of holding register definition arrays */
    const agile_modbus_slave_util_map_t *tab_input_registers;                                 /**< Input register definition array */
    int nb_input_registers;                                                                   /**<Input register definition array number */
    int (*addr_check)(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info);       /**< Address checking interface */
    int (*special_function)(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info); /**<Special function code processing interface */
    int (*done)(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, int ret);    /**< Processing end interface */
} agile_modbus_slave_util_t;

/**
 * @}
 */

/** @addtogroup SLAVE_UTIL_Exported_Functions
 * @{
 */
int agile_modbus_slave_util_callback(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, const void *data);
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __PKG_AGILE_MODBUS_SLAVE_UTIL_H */
