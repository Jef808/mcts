
set( bt_SOURCES
  breakthrough.cpp )

add_executable( bt
  ${bt_SOURCES} )

target_include_directories( bt
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR},
  ${mcts_source_DIR} )
