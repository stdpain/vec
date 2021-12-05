cmake_minimum_required (VERSION 3.8)


add_library(liblua STATIC IMPORTED)
set_target_properties(liblua PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/install/lib/liblua.lib)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/install/lib/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/install/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

FILE(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_executable (LuaBinder "LuaBinder.cpp" ${HEADERS})
target_link_libraries(LuaBinder liblua)

add_compile_options(-std=c++17)