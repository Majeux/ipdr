option(DO_LOG "Produce logs in logs/ folder" OFF)
option(DO_STATS "Collect statistics vor pdr" OFF)
option(DEBUG "turn off O3 and turn on assertions" OFF)

include(ExternalProject)
cmake_minimum_required(VERSION 3.8)
project("pebbling-pdr")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../)

find_package(cxxopts CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(ghc_filesystem CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Z3 CONFIG REQUIRED)

file(GLOB SOURCES "src/*.cpp")
file(GLOB MODEL_SOURCES "src/model/*.cpp")
file(GLOB PDR_MODEL_SOURCES "src/model/pdr/*.cpp")
file(GLOB PEBBLING_MODEL_SOURCES "src/model/pebbling/*.cpp")
file(GLOB PETERSON_MODEL_SOURCES "src/model/peterson/*.cpp")
file(GLOB ALGO_SOURCES "src/algo/*.cpp")
file(GLOB SOLVER_SOURCES "src/solver/*.cpp")
file(GLOB AUX_SOURCES "src/auxiliary/*.cpp")
file(GLOB TEST_SOURCES "src/testing/*.cpp")

add_executable(
  pebbling-pdr
  ${SOURCES}
  ${MODEL_SOURCES}
  ${PDR_MODEL_SOURCES}
  ${PEBBLING_MODEL_SOURCES}
  ${PETERSON_MODEL_SOURCES}
  ${ALGO_SOURCES}
  ${AUX_SOURCES}
  ${TEST_SOURCES})

# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(STATUS "GCC detected, adding compile flags")
  if(DEBUG)
    message(STATUS "! compiling without -O3")
    set(OPT "")
  else()
    target_compile_definitions(pebbling-pdr PRIVATE)
    message(STATUS "! compiling with -O3")
    target_compile_definitions(pebbling-pdr PRIVATE NDEBUG)
    message(STATUS "! compiling with NDEBUG")
    set(OPT "-O3")
  endif(DEBUG)

  set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -Wno-unknown-pragmas ${OPT}")

endif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

if(DO_LOG)
  target_compile_definitions(pebbling-pdr PRIVATE LOG)
  target_compile_definitions(pebbling-pdr PRIVATE Z3_ENABLE_TRACING_FOR_NON_DEBUG)
  message(STATUS "! logging turned on")
else()
  message(STATUS "! logging turned off")
endif(DO_LOG)

if(DO_STATS)
  target_compile_definitions(pebbling-pdr PRIVATE STATS)
  target_compile_definitions(pebbling-pdr PRIVATE Z3_ENABLE_TRACING_FOR_NON_DEBUG)
  message(STATUS "! logging turned on")
else()
  message(STATUS "! logging turned off")
endif(DO_STATS)

target_include_directories(
  pebbling-pdr
  PRIVATE inc
          inc/auxiliary
          inc/algo
          inc/solver
          inc/testing
          inc/model
          inc/model/pdr
          inc/model/pebbling
          inc/model/peterson)
target_include_directories(
  pebbling-pdr SYSTEM PRIVATE inc/ext/text-table inc/ext/tabulate/include
                              inc/ext/mockturtle/include)

add_library(fmt INTERFACE) # override mockturtle's local version with current
target_link_libraries(fmt INTERFACE fmt::fmt)
add_subdirectory(inc/ext/mockturtle/include)
add_subdirectory(inc/ext/mockturtle/lib)

# vcpkg libraries
target_link_libraries(pebbling-pdr PRIVATE cxxopts::cxxopts)
target_link_libraries(pebbling-pdr INTERFACE fmt)
target_link_libraries(pebbling-pdr PRIVATE ghcFilesystem::ghc_filesystem)
target_link_libraries(pebbling-pdr PRIVATE mockturtle)
target_link_libraries(pebbling-pdr PRIVATE spdlog::spdlog
                                           spdlog::spdlog_header_only)
target_link_libraries(pebbling-pdr PRIVATE z3::libz3)

# manual
target_link_libraries(pebbling-pdr PRIVATE gvc)
target_link_libraries(pebbling-pdr PRIVATE cgraph)
target_link_libraries(pebbling-pdr PRIVATE cdt)
