ADD_LIBRARY(
  tinytsumego2
  bloom.c
  full_solver.c
  partial_solver.c
  scoring.c
  state.c
  stones.c
  stones16.c
  tablebase/base.c
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
