cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(Washizu)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

set(src_dir ../)

include_directories(${src_dir})

add_library(Mahjong STATIC ${src_dir}/mahjong/mahjong.c
    ${src_dir}/mahjong/yaku.c ${src_dir}/mahjong/interaction.c)

add_library(Renderer STATIC ${src_dir}/renderer/camera.cpp
    ${src_dir}/renderer/mesh.cpp ${src_dir}/renderer/shader.cpp
    ${src_dir}/renderer/texture.cpp ${src_dir}/input/input.cpp)

target_link_libraries(Renderer PRIVATE GL GLEW glfw png)

add_executable(Server ${src_dir}/server/main.cxx ${src_dir}/server/deck.cpp
    ${src_dir}/server/game.cpp ${src_dir}/server/client.cpp
    ${src_dir}/server/extra.cpp)
add_executable(CLIClient ${src_dir}/client/cli.cxx)
add_executable(2DClient ${src_dir}/client/2d.cxx ${src_dir}/renderer/2d.cpp)

target_link_libraries(Server PRIVATE Mahjong pthread)
target_link_libraries(CLIClient PRIVATE Mahjong pthread)
target_link_libraries(2DClient PRIVATE Mahjong Renderer pthread)
