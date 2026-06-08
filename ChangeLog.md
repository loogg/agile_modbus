# Update record

## Agile Modbus 1.2.0 released

### Revise

2026-06-08: mengplus

* get/set 回调接口变更：`get(offset, len, buf, bufsz)` / `set(offset, len, buf, bufsz)`，支持局部读写
* 优化读性能：`map->get` 只读取所需范围，不再全量读取后丢弃
* 去除栈上 253 字节固定 map_buf，对齐修复
* 去除冗余 memset，C++ 保留字冲突修复
* slave_util_callback 增加 NULL 安全检查
* 新增 Makefile 构建系统：`make lib` / `make test` / `make size-report` / `make format-check`
* 新增 21 个 check 框架单元测试
* 新增 CI 工作流：format-check → build (gcc+clang) → memory-check (valgrind) → example-build
* 新增 ROM/RAM 占用报表

## Agile Modbus 1.1.0 released

### New function

2021-12-02: Ma Longwei

* Add Doxygen comments and generate documentation

2021-12-28: Ma Longwei

* Add RTU and TCP host examples
* Add RTU and TCP slave examples
* Add sample document

2022-01-08: Ma Longwei

* Add RTU point-to-point transmission file example
* Add RTU broadcast transmission file example

### Revise

2022-01-06: Ma Longwei

* Modify the slave example, RTU and TCP use the same slave callback
* TCP slave supports up to 5 client access
* The TCP slave will automatically disconnect if it does not receive the correct message within 10s.

2022-01-08: Ma Longwei

* Remove the length limit in receiving data judgment
* Remove `agile_modbus_serialize_raw_request`'s length limit on raw data

## Agile Modbus 1.1.1 released

### Revise

2022-06-22: Ma Longwei

* README.md adds a bootloader link that supports Modbus firmware upgrade based on RT-Thread on AT32F437
* Add HPM6750_Boot link
* Change LICENSE to `Apache-2.0`

## Agile Modbus 1.1.2 released

### New function

2022-07-28: Ma Longwei

* Provide simple slave access `agile_modbus_slave_util_callback` interface

### Revise

2022-07-28: Ma Longwei

* `agile_modbus_slave_handle` adds `slave callback private data` parameter
* `agile_modbus_slave_callback_t` adds `private data` parameter
* The slave example in `examples` uses the `agile_modbus_slave_util_callback` interface to implement register reading and writing

## Agile Modbus 1.1.3 released

### Revise

2022-11-22: Ma Longwei

* Writing a single register in `agile_modbus_slave_handle` will point the `slave_info.buf` pointer to the local variable address. This address will be used by other variables after turning on compiler optimization. Modify it to point to the global variable address within the function.

## Agile Modbus 1.1.4 released

### Revise

* fixed some warnings for some compiltion IDE such as ses

### New function

* Add API `agile_modbus_compute_response_length_from_request`: Obtain the length of the response from the slave based on the request
