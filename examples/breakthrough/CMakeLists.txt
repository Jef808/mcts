set( bt_SOURCES
  ${bt_DIR}/board.cpp
  ${bt_DIR}/breakthrough.cpp )

add_executable( bt ${bt_SOURCES} )
target_link_libraries( bt mcts )
target_include_directories( bt
  PUBLIC
  ${bt_DIR}
  ${mcts_source_DIR}
  ${mcts_utils_DIR}
  )
