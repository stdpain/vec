cmake_minimum_required (VERSION 3.8)

add_executable(test.out ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp)
target_link_libraries(test.out
    vec
    tcmalloc_and_profiler 
    glog 
    gtest 
    gmock 
    m 
    pthread
)

include_directories(../core/include)
include_directories(../thirdparty/phmap/parallel_hashmap/)
include_directories(../thirdparty/robin-map/include)
include_directories(../thirdparty/include)

FUNCTION(ADD_BENCH TEST_NAME)
    ADD_EXECUTABLE(${TEST_NAME}.out ${CMAKE_CURRENT_SOURCE_DIR}/bench/${TEST_NAME}.cpp)
    TARGET_LINK_LIBRARIES(${TEST_NAME}.out vec benchmark pthread)
ENDFUNCTION()

ADD_BENCH(bench_function_call)
ADD_BENCH(bench_gather)
ADD_BENCH(bench_filter)
ADD_BENCH(bench_binarycolumn)
ADD_BENCH(bench_map)

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/bench/bench_filter.cpp PROPERTIES COMPILE_FLAGS "-mavx2 -mbmi2")

add_executable(simd_test.out ${CMAKE_CURRENT_SOURCE_DIR}/simd_test.cpp)
target_link_libraries(simd_test.out 
    gtest 
    fmt
)