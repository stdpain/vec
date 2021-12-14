cmake_minimum_required (VERSION 3.8)

find_package(vec)

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
#target_link_libraries(test.out glog gtest gmock pthread)

add_executable(bench_function_call.out ${CMAKE_CURRENT_SOURCE_DIR}/bench/bench_function_call.cpp)
target_link_libraries(bench_function_call.out 
    benchmark 
    pthread
)

add_executable(simd_test.out ${CMAKE_CURRENT_SOURCE_DIR}/simd_test.cpp)
target_link_libraries(simd_test.out 
    gtest 
    fmt
)