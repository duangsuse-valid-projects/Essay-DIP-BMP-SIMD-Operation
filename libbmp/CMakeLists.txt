cmake_minimum_required(VERSION 3.0)

project (libbmp LANGUAGES C)

set (CMAKE_INCLUDE_CURRENT_DIR ON)
set (CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -std=c11)

add_library (libbmp bmp.c)


