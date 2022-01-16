cmake_minimum_required(VERSION 3.21)
project(Washizu)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

include_directories(src/)

add_library(Mahjong STATIC src/mahjong/mahjong.c src/mahjong/mahjong.h
    src/mahjong/yaku.c src/mahjong/yaku.h src/mahjong/interaction.c 
    src/mahjong/interaction.h)

add_executable(TestMahjong src/mahjong/test.c)
add_executable(Server src/server/deck.hpp src/server/deck.cpp 
    src/server/game.hpp src/server/game.cpp src/server/client.cpp
    src/server/main.cxx)
add_executable(DummyClient src/client/dummy.cxx)

target_link_libraries(TestMahjong PRIVATE Mahjong)
target_link_libraries(Server PRIVATE Mahjong pthread)
target_link_libraries(DummyClient PRIVATE pthread)