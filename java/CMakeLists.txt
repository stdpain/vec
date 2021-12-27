cmake_minimum_required (VERSION 3.8)
project(jni-java)

set(JAVA_HOME ${CMAKE_CURRENT_SOURCE_DIR}/java CACHE STRING INTERNAL)

include_directories(${JAVA_HOME}/include)
include_directories(${JAVA_HOME}/include/linux)

FILE(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_executable (jni_test ${CMAKE_CURRENT_SOURCE_DIR}/jni_test.cpp ${HEADERS})
target_link_libraries(jni_test jvm pthread)
target_link_directories(jni_test PUBLIC ${JAVA_HOME}/jre/lib/amd64/server/)


add_executable(jni-bench-test ${CMAKE_CURRENT_SOURCE_DIR}/bench-test.cpp ${HEADERS})
target_link_libraries(jni-bench-test jvm benchmark)
target_link_directories(jni-bench-test PUBLIC ${JAVA_HOME}/jre/lib/amd64/server/)

add_compile_options(-std=c++17)