#!/usr/bin/env bash
if [ ! $GCC_HOME ];then
    echo "Not Found GCC_HOME using default GCC"
else
    export CC=$GCC_HOME/bin/gcc
    export CXX=$GCC_HOME/bin/g++
    export PATH=$GCC_HOME/bin:$PATH
fi

BUILD_THREAD=12
#BUILD_TYPE=Debug
BUILD_TYPE=Release
BUILD_DIR=build_$BUILD_TYPE
DIR=$(cd $(dirname $0) && pwd ) 

export CMAKE_GENERATOR="Ninja"

rm -rf build
mkdir $DIR/$BUILD_DIR
cd $DIR/$BUILD_DIR && 
    cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -Dgperftools_enable_libunwind=NO \
        -Dgperftools_enable_frame_pointers=ON \
        -Dgperftools_build_benchmark=OFF \
        -DBUILD_TESTING=OFF \
        -DBENCHMARK_ENABLE_TESTING=OFF \
        -DFMT_INSTALL=ON \
        -DCMAKE_INSTALL_PREFIX=`pwd`/install .. \
        && cmake --build . --parallel $BUILD_THREAD && cmake --install .
