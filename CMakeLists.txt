# -*- CMake -*-
#
# mcts-impl/CMakeLists.txt
cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS -g)

project(MCTS)

add_library(mcts INTERFACE)
target_include_directories(mcts INTERFACE .)

add_library(tikz SHARED ../tikztreeold/dfs.cpp)
target_include_directories(tikz PUBLIC ../tikztreeold)

add_library(ttt examples/TTT.cpp)
target_include_directories(ttt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/examples)

add_library(mctsttt examples/mctsttt.cpp)
target_link_libraries(ttt mcts tikz)

add_executable(tttmain examples/tttmain.cpp examples/mctsttt.cpp examples/TTT.cpp)
target_link_libraries(tttmain mcts)
target_include_directories(tttmain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/examples)
#target_link_libraries(mctsttt ttt mcts)
