#!/usr/bin/env bash
export CC=/opt/compiler/gcc-10/bin/gcc
export CXX=/opt/compiler/gcc-10/bin/g++
export PATH=/opt/compiler/gcc-10/bin:$PATH
export LIBRT=/opt/compiler/gcc-10/lib64/librt.so

BUILD_THREAD=12
BUILD_TYPE=DEBUG
BUILD_DIR=build
DIR=$(cd $(dirname $0) && pwd ) 

#rm -rf build
mkdir $DIR/$BUILD_DIR
cd $DIR/$BUILD_DIR && 
    cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DCMAKE_BUILD_TYPE=DEBUG \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -Dgperftools_enable_libunwind=NO \
        -Dgperftools_enable_frame_pointers=ON \
        -DLIBRT=$LIBRT \
        -DCMAKE_INSTALL_PREFIX=`pwd`/install .. \
        && make -j $BUILD_THREAD  && make install
