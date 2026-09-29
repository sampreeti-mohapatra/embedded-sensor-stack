#pragma once
#include <csignal>
#include <cstdint>
#include <ctime>
#include <string>
#include <sys/stat.h>

namespace stack {

inline volatile std::sig_atomic_t g_stop = 0;

inline void on_signal(int) { g_stop = 1; }

// No SA_RESTART: blocking calls return EINTR so loops can notice g_stop.
inline void install_signal_handlers() {
    struct sigaction sa {};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

inline std::uint64_t now_ns() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
}

inline void sleep_ms(int ms) {
    timespec ts{ms / 1000, static_cast<long>(ms % 1000) * 1000000L};
    nanosleep(&ts, nullptr);  // interrupted by signals on purpose
}

inline void ensure_dir(const std::string& path) { mkdir(path.c_str(), 0755); }

}  // namespace stack
