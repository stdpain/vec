include_directories(../thirdparty/include)

add_executable(test_ring_buffer src/queue/test_ring_buffer.cpp)
add_executable(bench_ring_buffer src/queue/bench_ring_buffer.cpp)
target_link_libraries(test_ring_buffer
    vec
    glog 
    gtest 
    gmock 
    m 
    gtest_main
    pthread
)
target_link_libraries(bench_ring_buffer
    vec
    glog 
    gtest 
    gmock 
    m 
    gtest_main
    pthread
)