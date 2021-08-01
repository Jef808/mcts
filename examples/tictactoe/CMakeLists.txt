set( tictactoe_SOURCES
  ${tictactoe_DIR}/tictactoe.cpp )

add_executable( tictactoe ${tictactoe_SOURCES} )
target_link_libraries( tictactoe mcts )
target_include_directories( tictactoe
  PUBLIC
  ${tictactoe_DIR}
  ${mcts_source_DIR}
  ${mcts_util_DIR}
  )
