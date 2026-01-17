ADD_LIBRARY(
  tinytsumego2
  full_solver.c
  scoring.c
  state.c
  stones.c
  tablebase.c
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
