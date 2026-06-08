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
 *
 * Optimized: calls map->get(offset, need_len, ...) to read only the
 * required range instead of the entire map.
 */
static int read_registers(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, const agile_modbus_slave_util_t *slave_util)
{
    int function = slave_info->sft->function;
    int address = slave_info->address;
    int nb = slave_info->nb;
    int send_index = slave_info->send_index;
    const agile_modbus_slave_util_map_t *maps = NULL;
    int nb_maps = 0;
    int is_bit;

    switch (function) {
    case AGILE_MODBUS_FC_READ_COILS:
        maps = slave_util->tab_bits;
        nb_maps = slave_util->nb_bits;
        is_bit = 1;
        break;

    case AGILE_MODBUS_FC_READ_DISCRETE_INPUTS:
        maps = slave_util->tab_input_bits;
        nb_maps = slave_util->nb_input_bits;
        is_bit = 1;
        break;

    case AGILE_MODBUS_FC_READ_HOLDING_REGISTERS:
        maps = slave_util->tab_registers;
        nb_maps = slave_util->nb_registers;
        is_bit = 0;
        break;

    case AGILE_MODBUS_FC_READ_INPUT_REGISTERS:
        maps = slave_util->tab_input_registers;
        nb_maps = slave_util->nb_input_registers;
        is_bit = 0;
        break;

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
        int need_len = address + nb - now_address;
        if (need_len > map_len)
            need_len = map_len;

        if (map->get) {
            int offset = now_address - map->start_addr;
            if (is_bit) {
                uint8_t tmp[256];
                int tmp_len = need_len;
                if (tmp_len > (int)sizeof(tmp))
                    tmp_len = (int)sizeof(tmp);
                map->get(offset, tmp_len, tmp, tmp_len);
                for (int j = 0; j < tmp_len; j++)
                    agile_modbus_slave_io_set(ctx->send_buf + send_index, i + j, tmp[j]);
            } else {
                uint16_t tmp[128];
                int tmp_len = need_len;
                if (tmp_len > (int)(sizeof(tmp) / sizeof(tmp[0])))
                    tmp_len = (int)(sizeof(tmp) / sizeof(tmp[0]));
                map->get(offset, tmp_len, tmp, tmp_len * sizeof(uint16_t));
                for (int j = 0; j < tmp_len; j++)
                    agile_modbus_slave_register_set(ctx->send_buf + send_index, i + j, tmp[j]);
            }
        }

        now_address += map_len - 1;
        i += map_len - 1;
    }

    return 0;
}

/**
 * @brief   write register
 *
 * Read-modify-write pattern: loads the full map, applies changes to the
 * needed range, then writes back only that range via map->set(offset, ...).
 */
static int write_registers(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, const agile_modbus_slave_util_t *slave_util)
{
    int function = slave_info->sft->function;
    int address = slave_info->address;
    int nb = 0;
    const agile_modbus_slave_util_map_t *maps = NULL;
    int nb_maps = 0;
    int is_bit;
    (void)ctx;

    switch (function) {
    case AGILE_MODBUS_FC_WRITE_SINGLE_COIL:
    case AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS:
        maps = slave_util->tab_bits;
        nb_maps = slave_util->nb_bits;
        is_bit = 1;
        nb = (function == AGILE_MODBUS_FC_WRITE_SINGLE_COIL) ? 1 : slave_info->nb;
        break;

    case AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER:
    case AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
        maps = slave_util->tab_registers;
        nb_maps = slave_util->nb_registers;
        is_bit = 0;
        nb = (function == AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER) ? 1 : slave_info->nb;
        break;

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
        int need_len = address + nb - now_address;
        if (need_len > map_len)
            need_len = map_len;

        if (map->set) {
            int offset = now_address - map->start_addr;
            int map_total = map->end_addr - map->start_addr + 1;

            if (is_bit) {
                /* Coils: byte-aligned buffer */
                uint8_t buf[256];
                int buf_bytes = map_total;
                if (buf_bytes > (int)sizeof(buf))
                    buf_bytes = (int)sizeof(buf);

                memset(buf, 0, buf_bytes);
                if (map->get)
                    map->get(0, map_total, buf, buf_bytes);

                if (function == AGILE_MODBUS_FC_WRITE_SINGLE_COIL) {
                    buf[offset] = *((int *)slave_info->buf) ? 1 : 0;
                } else {
                    for (int j = 0; j < need_len; j++)
                        buf[offset + j] = agile_modbus_slave_io_get(slave_info->buf, i + j);
                }

                int rc = map->set(offset, need_len, buf + offset, need_len);
                if (rc != 0)
                    return rc;
            } else {
                /* Registers: uint16_t-aligned buffer */
                uint16_t buf[128];
                int buf_count = map_total;
                if (buf_count > (int)(sizeof(buf) / sizeof(buf[0])))
                    buf_count = (int)(sizeof(buf) / sizeof(buf[0]));

                memset(buf, 0, buf_count * sizeof(uint16_t));
                if (map->get)
                    map->get(0, map_total, buf, buf_count * sizeof(uint16_t));

                if (function == AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER) {
                    buf[offset] = *((int *)slave_info->buf);
                } else {
                    for (int j = 0; j < need_len; j++)
                        buf[offset + j] = agile_modbus_slave_register_get(slave_info->buf, i + j);
                }

                int rc = map->set(offset, need_len, buf + offset, need_len * sizeof(uint16_t));
                if (rc != 0)
                    return rc;
            }
        }

        now_address += map_len - 1;
        i += map_len - 1;
    }

    return 0;
}

/**
 * @brief   mask write register
 *
 * Reads only the target register, applies mask, writes back only that register.
 */
static int mask_write_register(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, const agile_modbus_slave_util_t *slave_util)
{
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
        int offset = address - map->start_addr;
        uint16_t data;
        uint16_t and_mask;
        uint16_t or_mask;

        if (map->get) {
            map->get(offset, 1, &data, sizeof(data));
        } else {
            data = 0;
        }

        and_mask = (slave_info->buf[0] << 8) + slave_info->buf[1];
        or_mask = (slave_info->buf[2] << 8) + slave_info->buf[3];
        data = (data & and_mask) | (or_mask & (~and_mask));

        int rc = map->set(offset, 1, &data, sizeof(data));
        if (rc != 0)
            return rc;
    }

    return 0;
}

/**
 * @brief   Write and read registers
 *
 * Write phase: read-modify-write pattern with partial set.
 * Read phase: partial get optimization.
 */
static int write_read_registers(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, const agile_modbus_slave_util_t *slave_util)
{
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
        int need_len = address_write + nb_write - now_address;
        if (need_len > map_len)
            need_len = map_len;

        if (map->set) {
            int offset = now_address - map->start_addr;
            int map_total = map->end_addr - map->start_addr + 1;
            uint16_t buf[128];
            int buf_count = map_total;
            if (buf_count > (int)(sizeof(buf) / sizeof(buf[0])))
                buf_count = (int)(sizeof(buf) / sizeof(buf[0]));

            memset(buf, 0, buf_count * sizeof(uint16_t));
            if (map->get)
                map->get(0, map_total, buf, buf_count * sizeof(uint16_t));

            for (int j = 0; j < need_len; j++)
                buf[offset + j] = agile_modbus_slave_register_get(slave_info->buf + 7, i + j);

            int rc = map->set(offset, need_len, buf + offset, need_len * sizeof(uint16_t));
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
        int need_len = address + nb - now_address;
        if (need_len > map_len)
            need_len = map_len;

        if (map->get) {
            int offset = now_address - map->start_addr;
            uint16_t tmp[128];
            int tmp_len = need_len;
            if (tmp_len > (int)(sizeof(tmp) / sizeof(tmp[0])))
                tmp_len = (int)(sizeof(tmp) / sizeof(tmp[0]));
            map->get(offset, tmp_len, tmp, tmp_len * sizeof(uint16_t));
            for (int j = 0; j < tmp_len; j++)
                agile_modbus_slave_register_set(ctx->send_buf + send_index, i + j, tmp[j]);
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

int agile_modbus_slave_util_callback(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, const void *data)
{
    int function = slave_info->sft->function;
    int ret = 0;
    const agile_modbus_slave_util_t *slave_util = (const agile_modbus_slave_util_t *)data;

    if (slave_util == NULL || slave_info == NULL)
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
