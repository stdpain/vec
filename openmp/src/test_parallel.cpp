#include <iostream>
#include <parallel/algorithm>
// https://gcc.gnu.org/onlinedocs/libstdc++/manual/parallel_mode_using.html
#include <omp.h>

#include <chrono>
#include <sstream>
#include <string>
#include <thread>

int main() {
    std::cout << omp_get_num_procs() << std::endl;
    std::cout << omp_get_num_threads() << std::endl;
    std::vector<int> values(4096);
    __gnu_parallel::sort(values.begin(), values.end());

    int x = 0;
    int y = 100;
#pragma omp parallel private(x) firstprivate(y)
    {
        // std::cout << x << std::endl; x is private and not inited
        x = 4096;
        std::cout << std::to_string(x) + '\n';   // now x is inited
        std::cout << std::to_string(y++) + "\n"; // y is private
#pragma omp single
        {
            std::cout << "only output once\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

#pragma omp sections
        {
#pragma omp section
            { std::cout << "section 0\n"; }
#pragma omp section
            { std::cout << "section 1\n"; }
#pragma omp section
            { std::this_thread::sleep_for(std::chrono::seconds(1)); }
#pragma omp section
            { std::cout << "section 2\n"; }
        }
    }
    // loop construct
    int m = 1026;
#pragma omp parallel for
    for (int i = 0; i < 20; ++i) {
        // current thread id
        std::stringstream ss;
        ss << std::this_thread::get_id() << "::" << omp_get_thread_num() << ":" << i;
#pragma omp critical
        { std::cout << ss.str() << std::endl; }
#pragma omp atomic
        m += i;
    }

    return 0;
}
