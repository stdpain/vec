#include <benchmark/benchmark.h>
#include <malloc.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <atomic>
#include <csignal>
#include <ctime>

class SignalTimerGuard {
public:
    SignalTimerGuard(int64_t timeout_ms) {
        if (timeout_ms > 0) {
            timer_t timerid;
            sigevent sev;
            itimerspec its;

            sev.sigev_notify = SIGEV_THREAD;
            sev.sigev_notify_function = dump_trace_info;
            sev.sigev_value.sival_int = static_cast<int>(syscall(SYS_gettid));
            sev.sigev_notify_attributes = nullptr;

            if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) != 0) {
                return;
            }

            its.it_value.tv_sec = timeout_ms / 1000;
            its.it_value.tv_nsec = (timeout_ms % 1000) * 1000000;
            its.it_interval.tv_sec = 0;
            its.it_interval.tv_nsec = 0;

            if (timer_settime(timerid, 0, &its, nullptr) != 0) {
                return;
            }

            _timer = timerid;
        }
    }

    ~SignalTimerGuard() {
        if (_timer) {
            timer_delete(_timer);
        }
    }

private:
    timer_t _timer{};

    static void dump_trace_info(union sigval sv) {}
};

static void signalTimer(benchmark::State& state) {
    for (auto _ : state) {
        SignalTimerGuard guard(1000);
    }
}

BENCHMARK(signalTimer);
BENCHMARK_MAIN();
