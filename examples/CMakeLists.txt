set( examples
  bt
  oware
  tictactoe
  cf )

set( bt_DIR
  ${mcts_examples_DIR}/breakthrough )

set( oware_DIR
  ${mcts_examples_DIR}/oware )

set( tictactoe_DIR
  ${mcts_examples_DIR}/tictactoe )

#set( cf_DIR
#  ${mcts_examples_DIR}/connect-four )

add_subdirectory( ${bt_DIR} )
add_subdirectory( ${oware_DIR} )
add_subdirectory( ${tictactoe_DIR} )
#add_subdirectory( ${cf_DIR} )
