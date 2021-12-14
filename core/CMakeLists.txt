cmake_minimum_required (VERSION 3.8)

FILE(GLOB VEC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)
SET(VEC_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(vec ${VEC_SRC})
target_include_directories(vec PUBLIC ${VEC_HEADER_DIR})