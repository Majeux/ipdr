# CMakeList.txt : CMake project for pebbling-pdr, include source and define
# project specific logic here.
#
OPTION(DO_LOG "Produce logs in logs/ folder" OFF) # Enabled by default

cmake_minimum_required (VERSION 3.8)
project ("pebbling-pdr")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../ )

# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "GCC detected, adding compile flags")
    set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -Wno-unknown-pragmas -O3")
endif(CMAKE_COMPILER_IS_GNUCXX)

find_package(fmt CONFIG REQUIRED)
find_package(Z3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(ghc_filesystem CONFIG REQUIRED)

file(GLOB SOURCES "src/*.cpp")
file(GLOB MODEL_SOURCES "src/model/*.cpp")
file(GLOB ALGO_SOURCES "src/algo/*.cpp")

add_executable (pebbling-pdr ${SOURCES} ${MODEL_SOURCES} ${ALGO_SOURCES})

if (DO_LOG)
	target_compile_definitions(pebbling-pdr PRIVATE LOG)
  message(STATUS "! logging turned on")
else ()
    message(STATUS "! logging turned off")
endif (DO_LOG)

target_include_directories(pebbling-pdr PRIVATE inc inc/auxiliary inc/model inc/algo inc/testing)
target_include_directories(pebbling-pdr PRIVATE inc/ext/text-table inc/ext/mockturtle/include)
add_subdirectory(inc/ext/mockturtle/include)
add_subdirectory(inc/ext/mockturtle/lib)

target_link_libraries(pebbling-pdr PRIVATE mockturtle)
target_link_libraries(pebbling-pdr PRIVATE ghcFilesystem::ghc_filesystem)
target_link_libraries(pebbling-pdr PRIVATE fmt::fmt-header-only)
target_link_libraries(pebbling-pdr PRIVATE z3::libz3)
target_link_libraries(pebbling-pdr PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(pebbling-pdr PRIVATE cxxopts::cxxopts)

# link graphviz on windows. assuming it is the default location
if (WIN32)
    target_link_libraries(pebbling-pdr PRIVATE "C:/Program Files/Graphviz/lib/gvc.lib")
    target_link_libraries(pebbling-pdr PRIVATE "C:/Program Files/Graphviz/lib/cgraph.lib")
    target_link_libraries(pebbling-pdr PRIVATE "C:/Program Files/Graphviz/lib/cdt.lib")
    target_include_directories(pebbling-pdr PRIVATE "C:/Program Files/Graphviz/include/")
endif()

# link graphviz on linux
if (UNIX) 
    target_link_libraries(pebbling-pdr PRIVATE gvc)
    target_link_libraries(pebbling-pdr PRIVATE cgraph)
    target_link_libraries(pebbling-pdr PRIVATE cdt)
endif()
