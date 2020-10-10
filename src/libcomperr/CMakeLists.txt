cmake_minimum_required(VERSION 3.17)
project(comperr C)

set(CMAKE_C_STANDARD 99)

add_library(comperr lib/comperr.c lib/comperr.h)
target_include_directories(comperr PUBLIC lib)

add_executable(comperr-test test.c)
target_link_libraries(comperr-test comperr)
