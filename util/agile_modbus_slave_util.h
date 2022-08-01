/**
 * @file    agile_modbus_slave_util.h
 * @brief   Agile Modbus 软件包提供的简易从机接入头文件
 * @author  马龙伟 (2544047213@qq.com)
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
 * @brief   从机寄存器映射结构体
 */
typedef struct agile_modbus_slave_util_map {
    int start_addr;                                       /**< 起始地址 */
    int end_addr;                                         /**< 结束地址 */
    int (*get)(void *buf, int bufsz);                     /**< 获取寄存器数据接口 */
    int (*set)(int index, int len, void *buf, int bufsz); /**< 设置寄存器数据接口 */
} agile_modbus_slave_util_map_t;

/**
 * @brief   从机功能结构体
 */
typedef struct agile_modbus_slave_util {
    const agile_modbus_slave_util_map_t *tab_bits;                                            /**< 线圈寄存器定义数组 */
    int nb_bits;                                                                              /**< 线圈寄存器定义数组数目 */
    const agile_modbus_slave_util_map_t *tab_input_bits;                                      /**< 离散量输入寄存器定义数组 */
    int nb_input_bits;                                                                        /**< 离散量输入寄存器定义数组数目 */
    const agile_modbus_slave_util_map_t *tab_registers;                                       /**< 保持寄存器定义数组 */
    int nb_registers;                                                                         /**< 保持寄存器定义数组数目 */
    const agile_modbus_slave_util_map_t *tab_input_registers;                                 /**< 输入寄存器定义数组 */
    int nb_input_registers;                                                                   /**< 输入寄存器定义数组数目 */
    int (*addr_check)(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info);       /**< 地址检查接口 */
    int (*special_function)(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info); /**< 特殊功能码处理接口 */
    int (*done)(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, int ret);    /**< 处理结束接口 */
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
