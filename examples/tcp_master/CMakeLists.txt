cmake_minimum_required(VERSION 3.0)

project(tcp_master)

file(GLOB SRCS *.c)

add_executable(TcpMaster ${SRCS})

target_link_libraries(TcpMaster PRIVATE Threads::Threads)
