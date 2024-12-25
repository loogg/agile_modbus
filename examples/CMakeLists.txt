cmake_minimum_required(VERSION 3.0)

project(example)

set(LIB_PATH ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(EXEC_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(COMMON_HEAD_PATH ../inc ../util common)

include_directories(${COMMON_HEAD_PATH})


set(MODBUS_LIB modbus)
set(COMMON_LIB common)

set(LIBRARY_OUTPUT_PATH ${LIB_PATH})
file(GLOB MODBUS_SRCS ../src/*.c ../util/*.c)
file(GLOB COMMON_SRCS common/*.c)

add_library(${MODBUS_LIB} STATIC ${MODBUS_SRCS})
add_library(${COMMON_LIB} STATIC ${COMMON_SRCS})


set(EXECUTABLE_OUTPUT_PATH ${EXEC_PATH})
link_directories(${LIB_PATH})
link_libraries(${MODBUS_LIB} ${COMMON_LIB})

add_subdirectory(rtu_broadcast)
add_subdirectory(rtu_master)
add_subdirectory(rtu_p2p)
add_subdirectory(slave)
add_subdirectory(tcp_master)
