FROM ubuntu
RUN cp /etc/apt/sources.list /etc/apt/sources.list.bak
RUN sed -i 's|security.ubuntu.com|mirrors.aliyun.com|g' /etc/apt/sources.list && sed -i 's|archive.ubuntu.com|mirrors.aliyun.com|g' /etc/apt/sources.list
RUN apt-get update 
RUN apt-get install vim nano wget curl netcat-traditional -y
RUN apt-get install locate git -y
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get install gcc-10 g++-10 gdb -y
RUN apt-get install flex bison make -y
RUN apt-get install python2 python3.8 -y
RUN apt-get install patchelf -y
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 10
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 10
ARG BUILD_PARALLEL=16
# build cmake
ARG CMAKE_VERSION=3.19.2
# https://github.com/Kitware/CMake/tag/download/v${CMAKE_VERSION}
COPY cmake-${CMAKE_VERSION}.tar.gz /root/cmake-${CMAKE_VERSION}.tar.gz
RUN cd /root/ && tar -zxf cmake-${CMAKE_VERSION}.tar.gz && cd /root/cmake-${CMAKE_VERSION} && ./configure --parallel=${BUILD_PARALLEL} -- -DCMAKE_USE_OPENSSL=OFF && make -j ${BUILD_PARALLEL} && make install
RUN rm -rf /root/cmake-${CMAKE_VERSION}
# build ccache distcc
RUN apt-get install ccache distcc -y
RUN apt-get install graphviz -y
# perf
RUN apt install linux-tools-common -y
