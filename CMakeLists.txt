cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS -g
  )

project(MCTS)

include_directories(.)

#add_executable(main main.cpp)

add_executable(ttt examples/TTT.cpp)
