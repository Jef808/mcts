cmake_minimum_required( VERSION 3.20 )
project( mcts LANGUAGES CXX )

set( CMAKE_EXPORT_COMPILE_COMMANDS on )
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED 17 )

set( root_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
set( source_DIR ${root_DIR}/src )
set( examples_DIR ${root_DIR}/examples )

add_subdirectory( ${examples_DIR} )
