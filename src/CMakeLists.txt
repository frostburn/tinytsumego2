ADD_LIBRARY(
  tinytsumego2
  bitmatrix.c
  bloom.c
  collection.c
  complete_reader.c
  complete_solver.c
  compressed_graph.c
  compressed_reader.c
  keyspace.c
  scoring.c
  shape.c
  state.c
  status.c
  stones.c
  stones16.c
  util.c
)
INSTALL(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/tinytsumego2
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
INSTALL(
  TARGETS tinytsumego2
  ARCHIVE
  DESTINATION lib
)
