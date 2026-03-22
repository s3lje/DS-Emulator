cmake_minimum_required(VERSION 3.20)
project(nds_emu)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(nds_emu ${SOURCES})
target_include_directories(nds_emu PRIVATE src ${SDL2_INCLUDE_DIRS})
target_link_libraries(nds_emu PRIVATE ${SDL2_LIBRARIES})
