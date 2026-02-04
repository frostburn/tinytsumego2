ADD_LIBRARY(
  tinytsumego2
  bitmatrix.c
  bloom.c
  complete_reader.c
  complete_solver.c
  full_reader.c
  full_solver.c
  keyspace.c
  partial_solver.c
  scoring.c
  shape.c
  state.c
  status.c
  stones.c
  stones16.c
  tablebase/base.c
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
