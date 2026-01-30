CMAKE_MINIMUM_REQUIRED(VERSION 3.25.1)
PROJECT(tinytsumego2 LANGUAGES C)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
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
    "${CMAKE_CURRENT_SOURCE_DIR}/jkiss/include"
)
LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})

ADD_SUBDIRECTORY(jkiss/src)
ADD_SUBDIRECTORY(src)

ENABLE_TESTING()
ADD_SUBDIRECTORY(test)

ADD_EXECUTABLE(demo demo.c)
TARGET_LINK_LIBRARIES(demo tinytsumego2)

ADD_EXECUTABLE(solver solver.c)
TARGET_LINK_LIBRARIES(solver tinytsumego2 m)

ADD_EXECUTABLE(ab_solver ab_solver.c)
TARGET_LINK_LIBRARIES(ab_solver jkiss tinytsumego2 m)

ADD_EXECUTABLE(mc_solver mc_solver.c)
TARGET_LINK_LIBRARIES(mc_solver jkiss tinytsumego2 m)

ADD_EXECUTABLE(enumerate_eyespaces enumerate_eyespaces.c)
TARGET_LINK_LIBRARIES(enumerate_eyespaces tinytsumego2)

ADD_EXECUTABLE(build_tablebase build_tablebase.c)
TARGET_LINK_LIBRARIES(build_tablebase tinytsumego2 m)

ADD_EXECUTABLE(verify_tablebase verify_tablebase.c)
TARGET_LINK_LIBRARIES(verify_tablebase tinytsumego2 m)

ADD_EXECUTABLE(explore_solution explore_solution.c)
TARGET_LINK_LIBRARIES(explore_solution tinytsumego2 m)
