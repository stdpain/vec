cmake_minimum_required (VERSION 3.8)

FUNCTION(ADD_TEST TEST_NAME)
    ADD_EXECUTABLE(${TEST_NAME}.out ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}.cpp)
    TARGET_LINK_LIBRARIES(${TEST_NAME}.out 
    vec 
    tcmalloc_and_profiler  
    glog  
    gtest  
    gmock  
    m  
    pthread)
ENDFUNCTION()

ADD_TEST(test)
ADD_TEST(test_sse_memcmp)
ADD_TEST(test_decimal_converter)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/test_sse_memcmp.cpp PROPERTIES COMPILE_FLAGS "-msse4.2 -mavx2 -fno-strict-aliasing")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/test_decimal_converter.cpp PROPERTIES COMPILE_FLAGS "-msse4.2 -mavx2 -fno-strict-aliasing")

include_directories(../core/include)
include_directories(../thirdparty/phmap/parallel_hashmap/)
include_directories(../thirdparty/robin-map/include)
include_directories(../thirdparty/include)
include_directories(../thirdparty/folly)
include_directories(../thirdparty/emhash/thirdparty/)

FUNCTION(ADD_BENCH TEST_NAME)
    ADD_EXECUTABLE(${TEST_NAME}.out ${CMAKE_CURRENT_SOURCE_DIR}/bench/${TEST_NAME}.cpp)
    TARGET_LINK_LIBRARIES(${TEST_NAME}.out vec benchmark pthread)
ENDFUNCTION()

ADD_BENCH(bench_function_call)
ADD_BENCH(bench_gather)
ADD_BENCH(bench_filter)
ADD_BENCH(bench_binarycolumn)
ADD_BENCH(bench_map)
ADD_BENCH(bench_mem_access)
ADD_BENCH(bench_pause)
ADD_BENCH(bench_fnv)
ADD_BENCH(bench_count_zero)
ADD_BENCH(bench_histogram)
ADD_BENCH(bench_sum_group_by)
ADD_BENCH(avx512_ht)
ADD_BENCH(bench_memcmp)
ADD_BENCH(bench_decimal_convert)
ADD_BENCH(bench_link)

add_library(call SHARED ${CMAKE_CURRENT_SOURCE_DIR}/bench/call.cpp)
TARGET_LINK_LIBRARIES(bench_link.out call benchmark pthread)

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/bench/bench_filter.cpp PROPERTIES COMPILE_FLAGS "-mavx2 -mbmi2 -mavx512f")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/bench/bench_count_zero.cpp PROPERTIES COMPILE_FLAGS "-msse4.2 -mavx2 -mbmi2")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/bench/bench_histogram.cpp PROPERTIES COMPILE_FLAGS "-mavx2 -mbmi2 -mavx512f")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/bench/avx512_ht.cpp PROPERTIES COMPILE_FLAGS "-mavx2 -mbmi2 -mavx512f")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/bench/bench_memcmp.cpp PROPERTIES COMPILE_FLAGS "-mavx2 -mbmi2 -mavx512f -mavx512bw -mavx512vl")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/bench/bench_decimal_convert.cpp PROPERTIES COMPILE_FLAGS "-mavx2")

add_executable(simd_test.out ${CMAKE_CURRENT_SOURCE_DIR}/simd_test.cpp)
target_link_libraries(simd_test.out 
    gtest 
    fmt
)