cmake_minimum_required(VERSION 3.10)

project(bitmaprevcolorsimd LANGUAGES C)

add_subdirectory(libbmp)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_C_FLAGS "-march=core2 -mmmx -msse")

set (CMAKE_THREAD_LIBS_INIT ON)
find_package(Threads REQUIRED)

add_executable(bitmaprevcolorsimd main.c)

target_link_libraries(bitmaprevcolorsimd libbmp Threads::Threads)

install(TARGETS bitmaprevcolorsimd RUNTIME DESTINATION bin)
