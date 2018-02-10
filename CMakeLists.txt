cmake_minimum_required(VERSION 3.1)
project(bubblesort C)

set(CMAKE_C_STANDARD 11)

add_executable(Sort Sort.c err.h err.c protocol.h)
target_link_libraries(Sort rt pthread)
add_executable(A A.c err.h err.c protocol.h)
target_link_libraries(A rt pthread)
