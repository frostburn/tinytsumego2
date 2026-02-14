CMAKE_MINIMUM_REQUIRED(VERSION 3.25.1)
PROJECT(tinytsumego2 LANGUAGES C)

set(VERSION_MAJOR 2)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
SET(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")

INCLUDE_DIRECTORIES(
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/api"
    "${CMAKE_CURRENT_SOURCE_DIR}/jkiss/include"
)
LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})

ADD_SUBDIRECTORY(jkiss/src)
ADD_SUBDIRECTORY(src)

ENABLE_TESTING()
ADD_SUBDIRECTORY(test)

ADD_EXECUTABLE(solver solver.c)
TARGET_LINK_LIBRARIES(solver tinytsumego2 m)

ADD_EXECUTABLE(dual_solver dual_solver.c)
TARGET_LINK_LIBRARIES(dual_solver tinytsumego2 m)

ADD_EXECUTABLE(tabulate_notchers tabulate_notchers.c)
TARGET_LINK_LIBRARIES(tabulate_notchers tinytsumego2 m)

ADD_EXECUTABLE(explore_solution explore_solution.c)
TARGET_LINK_LIBRARIES(explore_solution tinytsumego2 m)

ADD_EXECUTABLE(explore_dual explore_dual.c)
TARGET_LINK_LIBRARIES(explore_dual tinytsumego2 m)

CONFIGURE_FILE (api/tinytsumego2.h.in ${CMAKE_CURRENT_SOURCE_DIR}/api/tinytsumego2.h @ONLY)

ADD_LIBRARY(
    tinytsumego
    SHARED
    lib.c
    src/dual_solver.c
    src/keyspace.c
    src/scoring.c
    src/state.c
    src/stones.c
    src/stones16.c
    src/util.c
)
SET_TARGET_PROPERTIES(tinytsumego PROPERTIES
    SOVERSION 1
    PUBLIC_HEADER api/tinytsumego2.h)
TARGET_LINK_LIBRARIES(tinytsumego jkiss tinytsumego2 m)
