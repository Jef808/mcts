cmake_minimum_required( VERSION 3.20 )
project( mcts LANGUAGES CXX )

set( CMAKE_EXPORT_COMPILE_COMMANDS on )
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED 17 )
#set( CMAKE_BUILD_TYPE debug )
set( CMAKE_BUILD_TYPE release )

# This makes a big difference for the call to __builtin_popcountll
# to count the number of set bits in a Bitboard.
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native" )


set( mcts_root_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
set( mcts_source_DIR ${mcts_root_DIR}/src )
set( mcts_utils_DIR ${mcts_source_DIR}/utils )
set( mcts_examples_DIR ${mcts_root_DIR}/examples )

add_library( utils INTERFACE ${mcts_utils_DIR}/stopwatch.h )
target_include_directories( utils INTERFACE ${mcts_utils_DIR} )

add_library( mcts INTERFACE ${mcts_source_DIR}/mcts.h )
target_link_libraries( mcts INTERFACE utils )
target_include_directories( mcts INTERFACE ${mcts_DIR} )

add_subdirectory( ${mcts_examples_DIR} )
