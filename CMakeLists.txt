cmake_minimum_required(VERSION 4.2)
project(hw07 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

add_executable(hw07 server.c
        data_structures.c
)
