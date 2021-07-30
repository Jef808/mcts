
set( bt_SOURCES
  ${bt_DIR}/board.cpp,
  ${bt_DIR}/bitboard.cpp )

add_executable( bt
  breakthrough.cpp board.cpp bitboard.cpp )

target_include_directories( bt
  PUBLIC
  ${bt_DIR},
  ${mcts_source_DIR} )
