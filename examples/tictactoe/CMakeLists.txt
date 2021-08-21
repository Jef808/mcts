set( tictactoe_SOURCES
  ${tictactoe_DIR}/tictactoe.cpp
  )

add_library( ttt tictactoe.cpp )
target_include_directories( ttt
  PUBLIC
  ${tictactoe_DIR}
  ${mcts_source_DIR}
  )

add_executable( tictactoe ttt_main.cpp tictactoe.cpp )
target_link_libraries( tictactoe PUBLIC mcts )
target_include_directories( tictactoe
   PUBLIC
   ${tictactoe_DIR}
   ${mcts_source_DIR}
   ${mcts_utils_DIR}
   )
