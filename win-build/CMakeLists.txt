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
set (src_dir ../)

include_directories(/usr/x86_64-w64-mingw32/include/)
include_directories(${src_dir})

find_library(LIBOPENGL NAMES libopengl32.a PATHS /usr/x86_64-w64-mingw32/lib/)
find_library(LIBGDI NAMES libgdi32.a PATHS /usr/x86_64-w64-mingw32/lib/)
find_library(LIBGLFW NAMES libglfw3.a PATHS /usr/x86_64-w64-mingw32/lib/)
find_library(LIBGLEW NAMES libglew32.dll.a PATHS /usr/x86_64-w64-mingw32/lib/)
find_library(LIBPNG NAMES libpng.a PATHS /usr/x86_64-w64-mingw32/lib/)
find_library(LIBSSP NAMES libssp.a PATHS /usr/x86_64-w64-mingw32/lib/)
find_library(LIBZ NAMES libz.a PATHS /usr/x86_64-w64-mingw32/lib/)

add_library(Mahjong STATIC ${src_dir}/mahjong/mahjong.c
    ${src_dir}/mahjong/yaku.c ${src_dir}/mahjong/interaction.c)

add_library(Renderer STATIC ${src_dir}/renderer/camera.cpp
    ${src_dir}/renderer/mesh.cpp ${src_dir}/renderer/shader.cpp
    ${src_dir}/renderer/texture.cpp ${src_dir}/input/input.cpp)

target_link_libraries(Renderer INTERFACE ${LIBGLFW} ${LIBGLEW} ${LIBPNG} ${LIBSSP} ${LIBZ} ${LIBOPENGL} ${LIBGDI} )

add_executable(Server ${src_dir}/server/main.cxx ${src_dir}/server/deck.cpp
${src_dir}/server/game.cpp ${src_dir}/server/client.cpp ${src_dir}/server/extra.cpp)
add_executable(CLIClient ${src_dir}/client/cli.cxx)
add_executable(2DClient ${src_dir}/client/2d.cxx ${src_dir}/renderer/2d.cpp)

target_link_libraries(Server PRIVATE Mahjong pthread ws2_32)
target_link_libraries(CLIClient PRIVATE Mahjong pthread ws2_32)
target_link_libraries(2DClient PRIVATE Mahjong Renderer pthread ws2_32)
