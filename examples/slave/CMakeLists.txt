cmake_minimum_required(VERSION 3.0)

project(slave)

file(GLOB SRCS *.c)

add_executable(ModbusSlave ${SRCS})

target_link_libraries(ModbusSlave PRIVATE Threads::Threads)
