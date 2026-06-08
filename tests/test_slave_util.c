/**
 * @file    test_slave_util.c
 * @brief   Unit tests for agile_modbus_slave_util (check framework)
 */

#include <check.h>
#include <string.h>
#include "agile_modbus.h"
#include "agile_modbus_slave_util.h"

/* ============================================================
 * Test data
 * ============================================================ */

static uint8_t _coil_data[4] = {0, 1, 0, 1};
static uint8_t _disc_input_data[4] = {1, 0, 1, 0};
static uint16_t _holding_data[4] = {0x1111, 0x2222, 0x3333, 0x4444};
static uint16_t _input_data[4] = {0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD};

static uint16_t _large_data[64];
static int _large_get_call_count;

static int _addr_check_called;
static int _done_called;
static int _done_ret;
static int _special_called;

/* ============================================================
 * Callback implementations
 * ============================================================ */

static int coil_get(int offset, int len, void *buf, int bufsz)
{
    uint8_t *ptr = (uint8_t *)buf;
    for (int i = 0; i < len; i++)
        ptr[i] = _coil_data[offset + i];
    return 0;
}

static int coil_set(int offset, int len, void *buf, int bufsz)
{
    uint8_t *ptr = (uint8_t *)buf;
    for (int i = 0; i < len; i++)
        _coil_data[offset + i] = ptr[i];
    return 0;
}

static int disc_get(int offset, int len, void *buf, int bufsz)
{
    uint8_t *ptr = (uint8_t *)buf;
    for (int i = 0; i < len; i++)
        ptr[i] = _disc_input_data[offset + i];
    return 0;
}

static int hold_get(int offset, int len, void *buf, int bufsz)
{
    uint16_t *ptr = (uint16_t *)buf;
    for (int i = 0; i < len; i++)
        ptr[i] = _holding_data[offset + i];
    return 0;
}

static int hold_set(int offset, int len, void *buf, int bufsz)
{
    uint16_t *ptr = (uint16_t *)buf;
    for (int i = 0; i < len; i++)
        _holding_data[offset + i] = ptr[i];
    return 0;
}

static int input_get(int offset, int len, void *buf, int bufsz)
{
    uint16_t *ptr = (uint16_t *)buf;
    for (int i = 0; i < len; i++)
        ptr[i] = _input_data[offset + i];
    return 0;
}

static int large_get(int offset, int len, void *buf, int bufsz)
{
    _large_get_call_count++;
    uint16_t *ptr = (uint16_t *)buf;
    for (int i = 0; i < len; i++)
        ptr[i] = _large_data[offset + i];
    return 0;
}

static int large_set(int offset, int len, void *buf, int bufsz)
{
    uint16_t *ptr = (uint16_t *)buf;
    for (int i = 0; i < len; i++)
        _large_data[offset + i] = ptr[i];
    return 0;
}

/* ============================================================
 * Map definitions
 * ============================================================ */

static const agile_modbus_slave_util_map_t coil_maps[] = {
    {100, 103, coil_get, coil_set}};

static const agile_modbus_slave_util_map_t disc_maps[] = {
    {200, 203, disc_get, NULL}};

static const agile_modbus_slave_util_map_t hold_maps[] = {
    {300, 303, hold_get, hold_set}};

static const agile_modbus_slave_util_map_t input_maps[] = {
    {400, 403, input_get, NULL}};

static const agile_modbus_slave_util_map_t large_maps[] = {
    {0, 63, large_get, large_set}};

static const agile_modbus_slave_util_map_t multi_maps[] = {
    {100, 103, hold_get, hold_set},
    {200, 203, input_get, NULL},
};

/* ============================================================
 * Test infrastructure
 * ============================================================ */

#define SEND_BUFSZ 260
#define READ_BUFSZ 260

static agile_modbus_rtu_t _ctx_rtu;
static uint8_t _send_buf[SEND_BUFSZ];
static uint8_t _read_buf[READ_BUFSZ];

static void setup(void)
{
    agile_modbus_rtu_init(&_ctx_rtu, _send_buf, SEND_BUFSZ, _read_buf, READ_BUFSZ);
    agile_modbus_set_slave((agile_modbus_t *)&_ctx_rtu, 1);
    memset(_send_buf, 0, SEND_BUFSZ);
    memset(_read_buf, 0, READ_BUFSZ);

    _coil_data[0] = 0; _coil_data[1] = 1; _coil_data[2] = 0; _coil_data[3] = 1;
    _disc_input_data[0] = 1; _disc_input_data[1] = 0; _disc_input_data[2] = 1; _disc_input_data[3] = 0;
    _holding_data[0] = 0x1111; _holding_data[1] = 0x2222; _holding_data[2] = 0x3333; _holding_data[3] = 0x4444;
    _input_data[0] = 0xAAAA; _input_data[1] = 0xBBBB; _input_data[2] = 0xCCCC; _input_data[3] = 0xDDDD;

    for (int i = 0; i < 64; i++)
        _large_data[i] = (uint16_t)(i * 100);
    _large_get_call_count = 0;
    _addr_check_called = 0;
    _done_called = 0;
    _done_ret = -999;
    _special_called = 0;
}

/* RTU frame builder helpers */

static int build_read_request(uint8_t fc, uint16_t addr, uint16_t nb)
{
    _read_buf[0] = 1;
    _read_buf[1] = fc;
    _read_buf[2] = (uint8_t)(addr >> 8);
    _read_buf[3] = (uint8_t)(addr & 0xFF);
    _read_buf[4] = (uint8_t)(nb >> 8);
    _read_buf[5] = (uint8_t)(nb & 0xFF);
    return 6;
}

static int build_write_single_request(uint8_t fc, uint16_t addr, uint16_t value)
{
    _read_buf[0] = 1;
    _read_buf[1] = fc;
    _read_buf[2] = (uint8_t)(addr >> 8);
    _read_buf[3] = (uint8_t)(addr & 0xFF);
    _read_buf[4] = (uint8_t)(value >> 8);
    _read_buf[5] = (uint8_t)(value & 0xFF);
    return 6;
}

static int build_write_multiple_request(uint8_t fc, uint16_t addr, uint16_t nb, const uint16_t *values)
{
    _read_buf[0] = 1;
    _read_buf[1] = fc;
    _read_buf[2] = (uint8_t)(addr >> 8);
    _read_buf[3] = (uint8_t)(addr & 0xFF);
    _read_buf[4] = (uint8_t)(nb >> 8);
    _read_buf[5] = (uint8_t)(nb & 0xFF);
    _read_buf[6] = (uint8_t)(nb * 2);
    for (int i = 0; i < nb; i++) {
        _read_buf[7 + i * 2] = (uint8_t)(values[i] >> 8);
        _read_buf[7 + i * 2 + 1] = (uint8_t)(values[i] & 0xFF);
    }
    return 7 + nb * 2;
}

static int build_mask_write_request(uint16_t addr, uint16_t and_mask, uint16_t or_mask)
{
    _read_buf[0] = 1;
    _read_buf[1] = AGILE_MODBUS_FC_MASK_WRITE_REGISTER;
    _read_buf[2] = (uint8_t)(addr >> 8);
    _read_buf[3] = (uint8_t)(addr & 0xFF);
    _read_buf[4] = (uint8_t)(and_mask >> 8);
    _read_buf[5] = (uint8_t)(and_mask & 0xFF);
    _read_buf[6] = (uint8_t)(or_mask >> 8);
    _read_buf[7] = (uint8_t)(or_mask & 0xFF);
    return 8;
}

static int build_write_and_read_request(uint16_t read_addr, uint16_t read_nb,
                                        uint16_t write_addr, uint16_t write_nb,
                                        const uint16_t *write_values)
{
    _read_buf[0] = 1;
    _read_buf[1] = AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS;
    _read_buf[2] = (uint8_t)(read_addr >> 8);
    _read_buf[3] = (uint8_t)(read_addr & 0xFF);
    _read_buf[4] = (uint8_t)(read_nb >> 8);
    _read_buf[5] = (uint8_t)(read_nb & 0xFF);
    _read_buf[6] = (uint8_t)(write_addr >> 8);
    _read_buf[7] = (uint8_t)(write_addr & 0xFF);
    _read_buf[8] = (uint8_t)(write_nb >> 8);
    _read_buf[9] = (uint8_t)(write_nb & 0xFF);
    _read_buf[10] = (uint8_t)(write_nb * 2);
    for (int i = 0; i < write_nb; i++) {
        _read_buf[11 + i * 2] = (uint8_t)(write_values[i] >> 8);
        _read_buf[11 + i * 2 + 1] = (uint8_t)(write_values[i] & 0xFF);
    }
    return 11 + write_nb * 2;
}

static void append_crc(int pdu_len)
{
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < pdu_len; i++) {
        crc ^= (uint16_t)_read_buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    _read_buf[pdu_len] = (uint8_t)(crc & 0xFF);
    _read_buf[pdu_len + 1] = (uint8_t)(crc >> 8);
}

static int handle_request(int pdu_len, const agile_modbus_slave_util_t *slave_util)
{
    append_crc(pdu_len);
    int total_len = pdu_len + AGILE_MODBUS_RTU_CHECKSUM_LENGTH;
    return agile_modbus_slave_handle((agile_modbus_t *)&_ctx_rtu, total_len, 1,
                                     agile_modbus_slave_util_callback, slave_util, NULL);
}

/* ============================================================
 * Callbacks for addr_check / done / special_function
 * ============================================================ */

static int test_addr_check(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info)
{
    _addr_check_called = 1;
    return 0;
}

static int test_done_cb(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info, int ret)
{
    _done_called = 1;
    _done_ret = ret;
    return 0;
}

static int test_special_func(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info)
{
    _special_called = 1;
    return 0;
}

/* ============================================================
 * READ tests
 * ============================================================ */

START_TEST(test_read_coils_basic)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_bits = coil_maps, .nb_bits = 1,
    };
    int rsp_len = handle_request(build_read_request(AGILE_MODBUS_FC_READ_COILS, 100, 4), &util);
    ck_assert_int_gt(rsp_len, 0);
    int off = AGILE_MODBUS_RTU_HEADER_LENGTH;
    ck_assert_int_eq(_send_buf[off], AGILE_MODBUS_FC_READ_COILS);
    ck_assert_int_eq(_send_buf[off + 1], 1);
    ck_assert_int_eq(_send_buf[off + 2], 0x0A); /* bits: 0,1,0,1 => 0b1010 */
}
END_TEST

START_TEST(test_read_discrete_inputs_basic)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_input_bits = disc_maps, .nb_input_bits = 1,
    };
    int rsp_len = handle_request(build_read_request(AGILE_MODBUS_FC_READ_DISCRETE_INPUTS, 200, 4), &util);
    ck_assert_int_gt(rsp_len, 0);
    int off = AGILE_MODBUS_RTU_HEADER_LENGTH;
    ck_assert_int_eq(_send_buf[off], AGILE_MODBUS_FC_READ_DISCRETE_INPUTS);
    ck_assert_int_eq(_send_buf[off + 1], 1);
    ck_assert_int_eq(_send_buf[off + 2], 0x05); /* bits: 1,0,1,0 => 0b0101 */
}
END_TEST

START_TEST(test_read_holding_registers_basic)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_registers = hold_maps, .nb_registers = 1,
    };
    int rsp_len = handle_request(build_read_request(AGILE_MODBUS_FC_READ_HOLDING_REGISTERS, 300, 4), &util);
    ck_assert_int_gt(rsp_len, 0);
    int off = AGILE_MODBUS_RTU_HEADER_LENGTH;
    ck_assert_int_eq(_send_buf[off], AGILE_MODBUS_FC_READ_HOLDING_REGISTERS);
    ck_assert_int_eq(_send_buf[off + 1], 8);
    ck_assert_int_eq(_send_buf[off + 2], 0x11);
    ck_assert_int_eq(_send_buf[off + 3], 0x11);
    ck_assert_int_eq(_send_buf[off + 4], 0x22);
    ck_assert_int_eq(_send_buf[off + 5], 0x22);
}
END_TEST

START_TEST(test_read_input_registers_basic)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_input_registers = input_maps, .nb_input_registers = 1,
    };
    int rsp_len = handle_request(build_read_request(AGILE_MODBUS_FC_READ_INPUT_REGISTERS, 400, 4), &util);
    ck_assert_int_gt(rsp_len, 0);
    int off = AGILE_MODBUS_RTU_HEADER_LENGTH;
    ck_assert_int_eq(_send_buf[off], AGILE_MODBUS_FC_READ_INPUT_REGISTERS);
    ck_assert_int_eq(_send_buf[off + 1], 8);
    ck_assert_int_eq(_send_buf[off + 2], 0xAA);
    ck_assert_int_eq(_send_buf[off + 3], 0xAA);
}
END_TEST

START_TEST(test_read_partial_range)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_registers = hold_maps, .nb_registers = 1,
    };
    int rsp_len = handle_request(build_read_request(AGILE_MODBUS_FC_READ_HOLDING_REGISTERS, 301, 2), &util);
    ck_assert_int_gt(rsp_len, 0);
    int off = AGILE_MODBUS_RTU_HEADER_LENGTH;
    ck_assert_int_eq(_send_buf[off + 1], 4);
    ck_assert_int_eq(_send_buf[off + 2], 0x22);
    ck_assert_int_eq(_send_buf[off + 3], 0x22);
    ck_assert_int_eq(_send_buf[off + 4], 0x33);
    ck_assert_int_eq(_send_buf[off + 5], 0x33);
}
END_TEST

START_TEST(test_read_partial_get_params)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_registers = large_maps, .nb_registers = 1,
    };
    int rsp_len = handle_request(build_read_request(AGILE_MODBUS_FC_READ_HOLDING_REGISTERS, 10, 5), &util);
    ck_assert_int_gt(rsp_len, 0);
    ck_assert_int_eq(_large_get_call_count, 1);
    int off = AGILE_MODBUS_RTU_HEADER_LENGTH;
    ck_assert_int_eq(_send_buf[off + 2], (1000 >> 8));
    ck_assert_int_eq(_send_buf[off + 3], (1000 & 0xFF));
    ck_assert_int_eq(_send_buf[off + 4], (1100 >> 8));
    ck_assert_int_eq(_send_buf[off + 5], (1100 & 0xFF));
}
END_TEST

/* ============================================================
 * WRITE tests
 * ============================================================ */

START_TEST(test_write_single_coil_on)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_bits = coil_maps, .nb_bits = 1,
    };
    int rsp_len = handle_request(build_write_single_request(AGILE_MODBUS_FC_WRITE_SINGLE_COIL, 101, 0xFF00), &util);
    ck_assert_int_gt(rsp_len, 0);
    ck_assert_int_eq(_coil_data[1], 1);
}
END_TEST

START_TEST(test_write_single_coil_off)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_bits = coil_maps, .nb_bits = 1,
    };
    int rsp_len = handle_request(build_write_single_request(AGILE_MODBUS_FC_WRITE_SINGLE_COIL, 101, 0x0000), &util);
    ck_assert_int_gt(rsp_len, 0);
    ck_assert_int_eq(_coil_data[1], 0);
}
END_TEST

START_TEST(test_write_single_register)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_registers = hold_maps, .nb_registers = 1,
    };
    int rsp_len = handle_request(build_write_single_request(AGILE_MODBUS_FC_WRITE_SINGLE_REGISTER, 302, 0xABCD), &util);
    ck_assert_int_gt(rsp_len, 0);
    ck_assert_uint_eq(_holding_data[2], 0xABCD);
    ck_assert_uint_eq(_holding_data[0], 0x1111);
    ck_assert_uint_eq(_holding_data[3], 0x4444);
}
END_TEST

START_TEST(test_write_multiple_registers)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_registers = hold_maps, .nb_registers = 1,
    };
    uint16_t values[] = {0xEEEE, 0xFFFF};
    int rsp_len = handle_request(build_write_multiple_request(AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS, 301, 2, values), &util);
    ck_assert_int_gt(rsp_len, 0);
    ck_assert_uint_eq(_holding_data[0], 0x1111);
    ck_assert_uint_eq(_holding_data[1], 0xEEEE);
    ck_assert_uint_eq(_holding_data[2], 0xFFFF);
    ck_assert_uint_eq(_holding_data[3], 0x4444);
}
END_TEST

START_TEST(test_write_multiple_coils)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_bits = coil_maps, .nb_bits = 1,
    };
    _read_buf[0] = 1;
    _read_buf[1] = AGILE_MODBUS_FC_WRITE_MULTIPLE_COILS;
    _read_buf[2] = 0; _read_buf[3] = 100;
    _read_buf[4] = 0; _read_buf[5] = 3;
    _read_buf[6] = 1;
    _read_buf[7] = 0x05; /* bit0=1, bit1=0, bit2=1 */
    int rsp_len = handle_request(8, &util);
    ck_assert_int_gt(rsp_len, 0);
    ck_assert_int_eq(_coil_data[0], 1);
    ck_assert_int_eq(_coil_data[1], 0);
    ck_assert_int_eq(_coil_data[2], 1);
    ck_assert_int_eq(_coil_data[3], 1);
}
END_TEST

START_TEST(test_mask_write_register)
{
    setup();
    _holding_data[0] = 0x00FF;
    const agile_modbus_slave_util_t util = {
        .tab_registers = hold_maps, .nb_registers = 1,
    };
    int rsp_len = handle_request(build_mask_write_request(300, 0x00FF, 0xF000), &util);
    ck_assert_int_gt(rsp_len, 0);
    ck_assert_uint_eq(_holding_data[0], 0xF0FF);
}
END_TEST

START_TEST(test_write_and_read_registers)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_registers = hold_maps, .nb_registers = 1,
    };
    uint16_t write_vals[] = {0xBEEF};
    int rsp_len = handle_request(build_write_and_read_request(300, 3, 300, 1, write_vals), &util);
    ck_assert_int_gt(rsp_len, 0);
    ck_assert_uint_eq(_holding_data[0], 0xBEEF);
    int off = AGILE_MODBUS_RTU_HEADER_LENGTH;
    ck_assert_int_eq(_send_buf[off], AGILE_MODBUS_FC_WRITE_AND_READ_REGISTERS);
    ck_assert_int_eq(_send_buf[off + 1], 6);
    ck_assert_int_eq(_send_buf[off + 2], 0xBE);
    ck_assert_int_eq(_send_buf[off + 3], 0xEF);
}
END_TEST

START_TEST(test_write_preserves_unmapped)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_registers = hold_maps, .nb_registers = 1,
    };
    uint16_t values[] = {0x9999};
    int rsp_len = handle_request(build_write_multiple_request(AGILE_MODBUS_FC_WRITE_MULTIPLE_REGISTERS, 301, 1, values), &util);
    ck_assert_int_gt(rsp_len, 0);
    ck_assert_uint_eq(_holding_data[0], 0x1111);
    ck_assert_uint_eq(_holding_data[1], 0x9999);
    ck_assert_uint_eq(_holding_data[2], 0x3333);
    ck_assert_uint_eq(_holding_data[3], 0x4444);
}
END_TEST

/* ============================================================
 * Edge case tests
 * ============================================================ */

START_TEST(test_null_maps)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_bits = NULL, .nb_bits = 0,
    };
    int rsp_len = handle_request(build_read_request(AGILE_MODBUS_FC_READ_COILS, 100, 4), &util);
    ck_assert_int_gt(rsp_len, 0);
    ck_assert_int_eq(_send_buf[AGILE_MODBUS_RTU_HEADER_LENGTH], AGILE_MODBUS_FC_READ_COILS);
}
END_TEST

START_TEST(test_null_slave_util)
{
    setup();
    struct agile_modbus_slave_info dummy_info = {0};
    agile_modbus_sft_t sft = {0};
    dummy_info.sft = &sft;
    int ret = agile_modbus_slave_util_callback((agile_modbus_t *)&_ctx_rtu, &dummy_info, NULL);
    ck_assert_int_eq(ret, 0);
}
END_TEST

START_TEST(test_multi_map_read)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_registers = multi_maps, .nb_registers = 2,
    };
    int rsp_len = handle_request(build_read_request(AGILE_MODBUS_FC_READ_HOLDING_REGISTERS, 200, 2), &util);
    ck_assert_int_gt(rsp_len, 0);
    int off = AGILE_MODBUS_RTU_HEADER_LENGTH;
    ck_assert_int_eq(_send_buf[off + 1], 4);
    ck_assert_int_eq(_send_buf[off + 2], 0xAA);
    ck_assert_int_eq(_send_buf[off + 3], 0xAA);
    ck_assert_int_eq(_send_buf[off + 4], 0xBB);
    ck_assert_int_eq(_send_buf[off + 5], 0xBB);
}
END_TEST

/* ============================================================
 * Callback tests
 * ============================================================ */

START_TEST(test_addr_check_callback)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_registers = hold_maps, .nb_registers = 1,
        .addr_check = test_addr_check,
    };
    handle_request(build_read_request(AGILE_MODBUS_FC_READ_HOLDING_REGISTERS, 300, 1), &util);
    ck_assert_int_eq(_addr_check_called, 1);
}
END_TEST

START_TEST(test_done_callback)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .tab_registers = hold_maps, .nb_registers = 1,
        .done = test_done_cb,
    };
    handle_request(build_read_request(AGILE_MODBUS_FC_READ_HOLDING_REGISTERS, 300, 1), &util);
    ck_assert_int_eq(_done_called, 1);
    ck_assert_int_eq(_done_ret, 0);
}
END_TEST

START_TEST(test_special_function)
{
    setup();
    const agile_modbus_slave_util_t util = {
        .special_function = test_special_func,
    };
    agile_modbus_sft_t sft = {1, 0x41, 0};
    struct agile_modbus_slave_info slave_info = {0};
    slave_info.sft = &sft;
    int ret = agile_modbus_slave_util_callback((agile_modbus_t *)&_ctx_rtu, &slave_info, &util);
    ck_assert_int_eq(ret, 0);
    ck_assert_int_eq(_special_called, 1);
}
END_TEST

START_TEST(test_illegal_function_07)
{
    setup();
    const agile_modbus_slave_util_t util = {0};
    _read_buf[0] = 1;
    _read_buf[1] = AGILE_MODBUS_FC_READ_EXCEPTION_STATUS;
    int rsp_len = handle_request(2, &util);
    ck_assert_int_gt(rsp_len, 0);
    ck_assert_int_eq(_send_buf[AGILE_MODBUS_RTU_HEADER_LENGTH],
                     AGILE_MODBUS_FC_READ_EXCEPTION_STATUS + 0x80);
    ck_assert_int_eq(_send_buf[AGILE_MODBUS_RTU_HEADER_LENGTH + 1],
                     AGILE_MODBUS_EXCEPTION_ILLEGAL_FUNCTION);
}
END_TEST

/* ============================================================
 * Suite
 * ============================================================ */

static Suite *slave_util_suite(void)
{
    Suite *s = suite_create("slave_util");
    TCase *tc_read = tcase_create("read");
    TCase *tc_write = tcase_create("write");
    TCase *tc_edge = tcase_create("edge");
    TCase *tc_cb = tcase_create("callbacks");

    /* Read */
    tcase_add_test(tc_read, test_read_coils_basic);
    tcase_add_test(tc_read, test_read_discrete_inputs_basic);
    tcase_add_test(tc_read, test_read_holding_registers_basic);
    tcase_add_test(tc_read, test_read_input_registers_basic);
    tcase_add_test(tc_read, test_read_partial_range);
    tcase_add_test(tc_read, test_read_partial_get_params);
    suite_add_tcase(s, tc_read);

    /* Write */
    tcase_add_test(tc_write, test_write_single_coil_on);
    tcase_add_test(tc_write, test_write_single_coil_off);
    tcase_add_test(tc_write, test_write_single_register);
    tcase_add_test(tc_write, test_write_multiple_registers);
    tcase_add_test(tc_write, test_write_multiple_coils);
    tcase_add_test(tc_write, test_mask_write_register);
    tcase_add_test(tc_write, test_write_and_read_registers);
    tcase_add_test(tc_write, test_write_preserves_unmapped);
    suite_add_tcase(s, tc_write);

    /* Edge cases */
    tcase_add_test(tc_edge, test_null_maps);
    tcase_add_test(tc_edge, test_null_slave_util);
    tcase_add_test(tc_edge, test_multi_map_read);
    suite_add_tcase(s, tc_edge);

    /* Callbacks */
    tcase_add_test(tc_cb, test_addr_check_callback);
    tcase_add_test(tc_cb, test_done_callback);
    tcase_add_test(tc_cb, test_special_function);
    tcase_add_test(tc_cb, test_illegal_function_07);
    suite_add_tcase(s, tc_cb);

    return s;
}

int main(void)
{
    SRunner *sr = srunner_create(slave_util_suite());
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (failed == 0) ? 0 : 1;
}
