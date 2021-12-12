cmake_minimum_required(VERSION 3.12.0)
project(simu-bench)

add_definitions(-std=c++17)
# find_package(gtest)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
FILE(GLOB BENCH_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)
add_library(simu-bench ${BENCH_SRC})

FILE(GLOB BENCH_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cc)

add_executable(simu-bench-test ${BENCH_TEST_SRC})
target_link_libraries(simu-bench-test simu-bench gtest)
