cmake_minimum_required(VERSION 3.31)
project(grep_win C)

set(CMAKE_C_STANDARD 23)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

add_executable(grep_win main.c
        re.c
        re.h)
