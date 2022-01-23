cmake_minimum_required(VERSION 3.22)
project(Washizu)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

set(src_dir ../src)

include_directories(${src_dir})

add_library(Mahjong STATIC ${src_dir}/mahjong/mahjong.c
    ${src_dir}/mahjong/yaku.c ${src_dir}/mahjong/interaction.c)

add_executable(TestMahjong ${src_dir}/mahjong/test.c)
add_executable(Server ${src_dir}/server/main.cxx ${src_dir}/server/deck.cpp
${src_dir}/server/game.cpp ${src_dir}/server/client.cpp)
add_executable(DummyClient ${src_dir}/client/dummy.cxx)
add_executable(CLIClient ${src_dir}/client/cli.cxx)

target_link_libraries(TestMahjong PRIVATE Mahjong)
target_link_libraries(Server PRIVATE Mahjong pthread)
target_link_libraries(DummyClient PRIVATE pthread)
target_link_libraries(CLIClient PRIVATE Mahjong pthread)