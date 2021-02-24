CC=/opt/compiler/gcc-10/bin/gcc
CXX=/opt/compiler/gcc-10/bin/g++
#CXXFLAGS= -Wall -std=c++17 -O3 -fopt-info-vec-optimized -fopt-info-vec-missed -mavx
CXXFLAGS= -Wall -std=c++17 -O3 -fopt-info-vec-optimized -fopt-info-vec-missed 
#CXXFLAGS= -Wall -std=c++17 -O3 -fopt-info-vec-optimized -fopt-info-vec-missed -mavx -mavx512f
#CXXFLAGS= -Wall -std=c++17 -O3 -fopt-info-vec-optimized -mavx -mavx512f

VEC_INCLUDE=./include/

test.out : ./test/test.cpp ./include/vec/vec.h 
	$(CXX) $(CXXFLAGS) ./test/test.cpp -I $(VEC_INCLUDE) -o test.out
clean:
	rm ./test.out
