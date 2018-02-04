cmake_minimum_required(VERSION 3.1)
project(bubblesort C)

set(CMAKE_C_STANDARD 11)

add_executable(sorter sorter.c err.h err.c protocol.h)
target_link_libraries(sorter rt pthread)
add_executable(a a.c err.h err.c protocol.h)
target_link_libraries(a rt pthread)
