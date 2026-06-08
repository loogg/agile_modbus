cmake_minimum_required(VERSION 3.5)
project(agile_modbus_tests C)

set(CMAKE_C_STANDARD 99)

set(MODBUS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/agile_modbus.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/agile_modbus_rtu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/agile_modbus_tcp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../util/agile_modbus_slave_util.c
)

set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../util
)

add_executable(test_slave_util test_slave_util.c ${MODBUS_SRC})
target_include_directories(test_slave_util PRIVATE ${INCLUDE_DIRS})
target_link_libraries(test_slave_util check m subunit pthread rt)
target_compile_options(test_slave_util PRIVATE -Wall -Wextra -Wno-unused-parameter)

enable_testing()
add_test(NAME slave_util COMMAND test_slave_util)
