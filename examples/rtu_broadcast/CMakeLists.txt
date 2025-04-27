cmake_minimum_required(VERSION 3.0)

project(broadcast)

add_executable(broadcast_master broadcast_master.c)
add_executable(broadcast_slave broadcast_slave.c)

target_link_libraries(broadcast_master PRIVATE Threads::Threads)
target_link_libraries(broadcast_slave PRIVATE Threads::Threads)
