/**
 * @file    agile_modbus_slave_util.c
 * @brief   Agile Modbus software package provides simple slave access source files
 * @author  Ma Longwei (2544047213@qq.com)
 * @date    2022-07-28
 *
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2022 Ma Longwei.
 * All rights reserved.</center></h2>
 *
 */

#include "agile_modbus.h"
#include "agile_modbus_slave_util.h"
#include <string.h>

/** @addtogroup UTIL
 * @{
 */

/** @defgroup SLAVE_UTIL Slave Util
 * @{
 */

/** @defgroup SLAVE_UTIL_Private_Functions Slave Util Private Functions
 * @{
 */

/**
 * @brief   Get the mapping object from the mapping object array according to the register address
 * @param   maps mapping object array
 * @param   nb_maps number of arrays
 * @param   address register address
 * @return  !=NULL: mapping object; =NULL: failure
 */
static const agile_modbus_slave_util_map_t *get_map_by_addr(const agile_modbus_slave_util_map_t *maps, int nb_maps, int address)
{
    for (int i = 0; i < nb_maps; i++) {
        const agile_modbus_slave_util_map_t *map = &maps[i];
        if (address >= map->start_addr && address <= map->end_addr)
            return map;
    }

    return NULL;
}

/**
 * @brief   read register
 * @param   ctx modbus handle
 * @param   slave_info slave information body
 * @param   slave_util slave function structure
 * @return  =0: normal;
 *          <0: Abnormal
 *             (-AGILE_MODBUS_EXCEPTION_UNKNOW(-255): Unknown exception, the slave will not package the response data)
 *             (Other negative exception codes: package exception response data from the opportunity)
 */
static int read_registers(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, const agile_modbus_slave_util_t *slave_util)
{
    uint8_t map_buf[AGILE_MODBUS_MAX_PDU_LENGTH];
    int function = slave_info->sft->function;
    int address = slave_info->address;
    int nb = slave_info->nb;
    int send_index = slave_info->send_index;
    const agile_modbus_slave_util_map_t *maps = NULL;
    int nb_maps = 0;

    switch (function) {
    case AGILE_MODBUS_FC_READ_COILS: {
        maps = slave_util->tab_bits;
        nb_maps = slave_util->nb_bits;
    } break;

    case AGILE_MODBUS_FC_READ_DISCRETE_INPUTS: {
        maps = slave_util->tab_input_bits;
        nb_maps = slave_util->nb_input_bits;
    } break;

    case AGILE_MODBUS_FC_READ_HOLDING_REGISTERS: {
        maps = slave_util->tab_registers;
        nb_maps = slave_util->nb_registers;
    } break;

    case AGILE_MODBUS_FC_READ_INPUT_REGISTERS: {
        maps = slave_util->tab_input_registers;
        nb_maps = slave_util->nb_input_registers;
    } break;

    default:
        return -AGILE_MODBUS_EXCEPTION_ILLEGAL_FUNCTION;
    }

    if (maps == NULL)
        return 0;

    for (int now_address = address, i = 0; now_address < address + nb; now_address++, i++) {
        const agile_modbus_slave_util_map_t *map = get_map_by_addr(maps, nb_maps, now_address);
        if (map == NULL)
            continue;

        int map_len = map->end_addr - now_address + 1;
        if (map->get) {
            memset(map_buf, 0, sizeof(map_buf));
            map->get(map_buf, sizeof(map_buf));
            int index = now_address - map->start_addr;
            int need_len = address + nb - now_address;
            if (need_len > map_len) {
                need_len = map_len;
            }

            if (function == AGILE_MODBUS_FC_READ_COILS || function == AGILE_MODBUS_FC_READ_DISCRETE_INPUTS) {
                uint8_t *ptr = map_buf;
                for (int j = 0; j < need_len; j++) {
                    agile_modbus_slave_io_set(ctx->send_buf + send_index, i + j, ptr[index + j]);
                }
            } else {
                uint16_t *ptr = (uint16_t *)map_buf;
                for (int j = 0; j < need_len; j++) {
                    agile_modbus_slave_register_set(ctx->send_buf + send_index, i + j, ptr[index + j]);
                }
            }
        }

        now_address += map_len - 1;
        i += map_len - 1;
    }

    return 0;
}

/**
 * @brief   write register
 * @param   ctx modbus handle
 * @param   slave_info slave information body
 * @param   slave_util slave function structure
 * @return  =0: normal;
 *          <0: Abnormal
 *             (-AGILE_MODBUS_EXCEPTION_UNKNOW(-255): Unknown exception, the slave will not package the response data)
 *             (Other negative exception codes: package exception response data from the opportunity)
 */
static int write_registers(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, const agile_modbus_slave_util_t *slave_util)
{
    uint8_t map_buf[AGILE_MODBUS_MAX_PDU_LENGTH];
    int function = slave_info->sft->function;
    int address = slave_info->address;
    int nb = 0;
    const agile_modbus_slave_util_map_t *maps = NULL;
    int nb_maps = 0;
    (void)ctx;
    switch (function) {
    case AGILE_MODBUS_FC_WRITE_SINGLE_COIL:
    case AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS: {
        maps = slave_util->tab_bits;
        nb_maps = slave_util->nb_bits;
        if (function == AGILE_MODBUS_FC_WRITE_SINGLE_COIL) {
            nb = 1;
        } else {
            nb = slave_info->nb;
        }
    } break;

    case AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER:
    case AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS: {
        maps = slave_util->tab_registers;
        nb_maps = slave_util->nb_registers;
        if (function == AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER) {
            nb = 1;
        } else {
            nb = slave_info->nb;
        }
    } break;

    default:
        return -AGILE_MODBUS_EXCEPTION_ILLEGAL_FUNCTION;
    }

    if (maps == NULL)
        return 0;

    for (int now_address = address, i = 0; now_address < address + nb; now_address++, i++) {
        const agile_modbus_slave_util_map_t *map = get_map_by_addr(maps, nb_maps, now_address);
        if (map == NULL)
            continue;

        int map_len = map->end_addr - now_address + 1;
        if (map->set) {
            memset(map_buf, 0, sizeof(map_buf));
            if (map->get) {
                map->get(map_buf, sizeof(map_buf));
            }

            int index = now_address - map->start_addr;
            int need_len = address + nb - now_address;
            if (need_len > map_len) {
                need_len = map_len;
            }

            if (function == AGILE_MODBUS_FC_WRITE_SINGLE_COIL || function == AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS) {
                uint8_t *ptr = map_buf;
                if (function == AGILE_MODBUS_FC_WRITE_SINGLE_COIL) {
                    int data = *((int *)slave_info->buf);
                    ptr[index] = data;
                } else {
                    for (int j = 0; j < need_len; j++) {
                        uint8_t data = agile_modbus_slave_io_get(slave_info->buf, i + j);
                        ptr[index + j] = data;
                    }
                }
            } else {
                uint16_t *ptr = (uint16_t *)map_buf;
                if (function == AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER) {
                    int data = *((int *)slave_info->buf);
                    ptr[index] = data;
                } else {
                    for (int j = 0; j < need_len; j++) {
                        uint16_t data = agile_modbus_slave_register_get(slave_info->buf, i + j);
                        ptr[index + j] = data;
                    }
                }
            }

            int rc = map->set(index, need_len, map_buf, sizeof(map_buf));
            if (rc != 0)
                return rc;
        }

        now_address += map_len - 1;
        i += map_len - 1;
    }

    return 0;
}

/**
 * @brief   mask write register
 * @param   ctx modbus handle
 * @param   slave_info slave information body
 * @param   slave_util slave function structure
 * @return  =0: normal;
 *          <0: Abnormal
 *             (-AGILE_MODBUS_EXCEPTION_UNKNOW(-255): Unknown exception, the slave will not package the response data)
 *             (Other negative exception codes: package exception response data from the opportunity)
 */
static int mask_write_register(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, const agile_modbus_slave_util_t *slave_util)
{
    uint8_t map_buf[AGILE_MODBUS_MAX_PDU_LENGTH];
    int address = slave_info->address;
    const agile_modbus_slave_util_map_t *maps = slave_util->tab_registers;
    int nb_maps = slave_util->nb_registers;
    (void)ctx;
    if (maps == NULL)
        return 0;

    const agile_modbus_slave_util_map_t *map = get_map_by_addr(maps, nb_maps, address);
    if (map == NULL)
        return 0;

    if (map->set) {
        memset(map_buf, 0, sizeof(map_buf));
        if (map->get) {
            map->get(map_buf, sizeof(map_buf));
        }

        int index = address - map->start_addr;
        uint16_t *ptr = (uint16_t *)map_buf;
        uint16_t data = ptr[index];
        uint16_t and = (slave_info->buf[0] << 8) + slave_info->buf[1];
        uint16_t or = (slave_info->buf[2] << 8) + slave_info->buf[3];

        data = (data & and) | (or &(~and));
        ptr[index] = data;

        int rc = map->set(index, 1, map_buf, sizeof(map_buf));
        if (rc != 0)
            return rc;
    }

    return 0;
}

/**
 * @brief   Write and read registers
 * @param   ctx modbus handle
 * @param   slave_info slave information body
 * @param   slave_util slave function structure
 * @return  =0: normal;
 *          <0: Abnormal
 *             (-AGILE_MODBUS_EXCEPTION_UNKNOW(-255): Unknown exception, the slave will not package the response data)
 *             (Other negative exception codes: package exception response data from the opportunity)
 */
static int write_read_registers(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, const agile_modbus_slave_util_t *slave_util)
{
    uint8_t map_buf[AGILE_MODBUS_MAX_PDU_LENGTH];
    int address = slave_info->address;
    int nb = (slave_info->buf[0] << 8) + slave_info->buf[1];
    int address_write = (slave_info->buf[2] << 8) + slave_info->buf[3];
    int nb_write = (slave_info->buf[4] << 8) + slave_info->buf[5];
    int send_index = slave_info->send_index;

    const agile_modbus_slave_util_map_t *maps = slave_util->tab_registers;
    int nb_maps = slave_util->nb_registers;

    if (maps == NULL)
        return 0;

    /* Write first. 7 is the offset of the first values to write */
    for (int now_address = address_write, i = 0; now_address < address_write + nb_write; now_address++, i++) {
        const agile_modbus_slave_util_map_t *map = get_map_by_addr(maps, nb_maps, now_address);
        if (map == NULL)
            continue;

        int map_len = map->end_addr - now_address + 1;
        if (map->set) {
            memset(map_buf, 0, sizeof(map_buf));
            if (map->get) {
                map->get(map_buf, sizeof(map_buf));
            }

            int index = now_address - map->start_addr;
            uint16_t *ptr = (uint16_t *)map_buf;
            int need_len = address_write + nb_write - now_address;
            if (need_len > map_len) {
                need_len = map_len;
            }

            for (int j = 0; j < need_len; j++) {
                uint16_t data = agile_modbus_slave_register_get(slave_info->buf + 7, i + j);
                ptr[index + j] = data;
            }

            int rc = map->set(index, need_len, map_buf, sizeof(map_buf));
            if (rc != 0)
                return rc;
        }

        now_address += map_len - 1;
        i += map_len - 1;
    }

    /* and read the data for the response */
    for (int now_address = address, i = 0; now_address < address + nb; now_address++, i++) {
        const agile_modbus_slave_util_map_t *map = get_map_by_addr(maps, nb_maps, now_address);
        if (map == NULL)
            continue;

        int map_len = map->end_addr - now_address + 1;
        if (map->get) {
            memset(map_buf, 0, sizeof(map_buf));
            map->get(map_buf, sizeof(map_buf));
            int index = now_address - map->start_addr;
            uint16_t *ptr = (uint16_t *)map_buf;
            int need_len = address + nb - now_address;
            if (need_len > map_len) {
                need_len = map_len;
            }

            for (int j = 0; j < need_len; j++) {
                agile_modbus_slave_register_set(ctx->send_buf + send_index, i + j, ptr[index + j]);
            }
        }

        now_address += map_len - 1;
        i += map_len - 1;
    }

    return 0;
}

/**
 * @}
 */

/** @defgroup SLAVE_UTIL_Exported_Functions Slave Util Exported Functions
 * @{
 */

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
int agile_modbus_slave_util_callback(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, const void *data)
{
    int function = slave_info->sft->function;
    int ret = 0;
    const agile_modbus_slave_util_t *slave_util = (const agile_modbus_slave_util_t *)data;

    if (slave_util == NULL)
        return 0;

    if (slave_util->addr_check) {
        ret = slave_util->addr_check(ctx, slave_info);
        if (ret != 0)
            return ret;
    }

    switch (function) {
    case AGILE_MODBUS_FC_READ_COILS:
    case AGILE_MODBUS_FC_READ_DISCRETE_INPUTS:
    case AGILE_MODBUS_FC_READ_HOLDING_REGISTERS:
    case AGILE_MODBUS_FC_READ_INPUT_REGISTERS:
        ret = read_registers(ctx, slave_info, slave_util);
        break;

    case AGILE_MODBUS_FC_WRITE_SINGLE_COIL:
    case AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS:
    case AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER:
    case AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
        ret = write_registers(ctx, slave_info, slave_util);
        break;

    case AGILE_MODBUS_FC_MASK_WRITE_REGISTER:
        ret = mask_write_register(ctx, slave_info, slave_util);
        break;

    case AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS:
        ret = write_read_registers(ctx, slave_info, slave_util);
        break;

    default: {
        if (slave_util->special_function) {
            ret = slave_util->special_function(ctx, slave_info);
        } else {
            ret = -AGILE_MODBUS_EXCEPTION_ILLEGAL_FUNCTION;
        }
    } break;
    }

    if (slave_util->done) {
        slave_util->done(ctx, slave_info, ret);
    }

    return ret;
}

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
