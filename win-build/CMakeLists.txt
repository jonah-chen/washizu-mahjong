cmake_minimum_required(VERSION 3.22)
project(Washizu)

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

# where is the target environment located
set(CMAKE_FIND_ROOT_PATH  /usr/x86_64-w64-mingw32)

SET(CMAKE_RC_COMPILER    "x86_64-w64-mingw32-windres")
SET(CMAKE_RANLIB         "x86_64-w64-mingw32-ranlib")

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0A00 -static -O3")
set (src_dir ../src)

include_directories(/usr/x86_64-w64-mingw32/include/)

include_directories(${src_dir})

add_library(Mahjong STATIC ${src_dir}/mahjong/mahjong.c
    ${src_dir}/mahjong/yaku.c ${src_dir}/mahjong/interaction.c)

add_executable(TestMahjong ${src_dir}/mahjong/test.c)
add_executable(Server ${src_dir}/server/main.cxx ${src_dir}/server/deck.cpp
${src_dir}/server/game.cpp ${src_dir}/server/client.cpp)
add_executable(DummyClient ${src_dir}/client/dummy.cxx)
add_executable(CLIClient ${src_dir}/client/cli.cxx)

target_link_libraries(TestMahjong PRIVATE Mahjong)
target_link_libraries(Server PRIVATE Mahjong pthread ws2_32)
target_link_libraries(DummyClient PRIVATE pthread ws2_32)
target_link_libraries(CLIClient PRIVATE Mahjong pthread ws2_32)