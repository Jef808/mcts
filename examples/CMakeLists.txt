set( examples
  bt
  oware
  tictactoe )

set( bt_DIR
  ${mcts_examples_DIR}/breakthrough )

set( oware_DIR
  ${mcts_examples_DIR}/oware )

set( tictactoe_DIR
  ${mcts_examples_DIR}/tictactoe )

add_subdirectory( ${bt_DIR} )
add_subdirectory( ${oware_DIR} )
add_subdirectory( ${tictactoe_DIR} )
