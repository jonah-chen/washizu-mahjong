cmake_minimum_required(VERSION 3.21)
project(Washizu)

set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

include_directories(src/)

add_library(Mahjong STATIC src/mahjong/mahjong.c src/mahjong/mahjong.h
    src/mahjong/yaku.c src/mahjong/yaku.h)

add_executable(TestMahjong src/mahjong/test.c)

target_link_libraries(TestMahjong PRIVATE Mahjong)