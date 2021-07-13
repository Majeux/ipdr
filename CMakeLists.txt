# CMakeList.txt : CMake project for pebbling-pdr, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
project ("pebbling-pdr")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -O3")

find_package(fmt CONFIG REQUIRED)
find_package(Z3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

file(GLOB SOURCES "src/*.cpp")
file(GLOB MODEL_SOURCES "src/model/*.cpp")
file(GLOB ALGO_SOURCES "src/algo/*.cpp")

file(GLOB MODEL_HEADERS "inc/model/*.h")
file(GLOB AUX_HEADERS "inc/auxiliary/*.h")
file(GLOB ALGO_HEADERS "inc/algo/*.h")

add_executable (pebbling-pdr ${SOURCES} ${MODEL_SOURCES} ${ALGO_SOURCES})#  ${MODEL_HEADERS} ${AUX_HEADERS} ${ALGO_HEADERS})

target_include_directories(pebbling-pdr PRIVATE inc inc/auxiliary inc/model inc/algo)

target_link_libraries(pebbling-pdr PRIVATE fmt::fmt-header-only)
target_link_libraries(pebbling-pdr PRIVATE z3::libz3)
target_link_libraries(pebbling-pdr PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# make benchmarks available in out/build
add_custom_command(TARGET pebbling-pdr PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/benchmark/iscas85/bench $<TARGET_FILE_DIR:pebbling-pdr>/benchmark/iscas85/bench)
