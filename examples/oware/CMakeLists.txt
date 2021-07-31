
add_library( oware oware.cpp )
add_executable( oware_humanplay oware_human.cpp )
target_link_libraries( oware_humanplay PRIVATE oware )
target_include_directories( oware
  PUBLIC
  ${oware_DIR}
  ${mcts_source_DIR}
  ${mcts_utils_DIR} )

add_library( oware_mcts oware_mcts.cpp )
target_link_libraries( oware_mcts PUBLIC oware mcts )

add_executable( oware_main oware_main.cpp )
target_link_libraries( oware_main PRIVATE oware_mcts )

add_executable( oware_benchmark time_benchmarks.cpp )
target_link_libraries( oware_benchmark PRIVATE oware_mcts )
