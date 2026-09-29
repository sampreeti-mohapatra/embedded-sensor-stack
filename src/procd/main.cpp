// procd: receives readings, keeps a moving average, raises threshold alerts,
// tracks IPC latency, and publishes state as JSON for metricsd.
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>

#include "common/protocol.hpp"
#include "common/util.hpp"

using namespace stack;

namespace {

struct Channel {
    const char* name;
    double threshold;
    std::deque<double> window;
    double last = 0.0;
    unsigned long alerts = 0;

    double avg() const {
        if (window.empty()) return 0.0;
        double s = 0;
        for (double v : window) s += v;
        return s / window.size();
    }
    void push(double v) {
        last = v;
        window.push_back(v);
        if (window.size() > 10) window.pop_front();
    }
};

struct Stats {
    unsigned long messages = 0;
    double lat_sum_us = 0.0, lat_max_us = 0.0;
};

void write_state(const Channel* ch, int n, const Stats& st) {
    std::string tmp = std::string(kStateFile) + ".tmp";
    FILE* f = std::fopen(tmp.c_str(), "w");
    if (!f) return;
    std::fprintf(f, "{\n");
    for (int i = 0; i < n; ++i)
        std::fprintf(f,
            "  \"%s\": {\"last\": %.2f, \"avg\": %.2f, \"threshold\": %.1f, \"alerts\": %lu},\n",
            ch[i].name, ch[i].last, ch[i].avg(), ch[i].threshold, ch[i].alerts);
    std::fprintf(f,
        "  \"ipc_latency_us\": {\"avg\": %.1f, \"max\": %.1f},\n  \"messages\": %lu\n}\n",
        st.messages ? st.lat_sum_us / st.messages : 0.0, st.lat_max_us, st.messages);
    std::fclose(f);
    std::rename(tmp.c_str(), kStateFile);  // atomic replace
}

}  // namespace

int main() {
    openlog("procd", LOG_PID, LOG_DAEMON);
    install_signal_handlers();
    ensure_dir(kSockDir);

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) { syslog(LOG_ERR, "socket: %s", std::strerror(errno)); return 1; }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, kProcdSock, sizeof(addr.sun_path) - 1);
    unlink(kProcdSock);
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        syslog(LOG_ERR, "bind: %s", std::strerror(errno));
        return 1;
    }

    Channel ch[2] = {{"temperature", 35.0, {}}, {"humidity", 80.0, {}}};
    Stats st;
    syslog(LOG_INFO, "listening on %s", kProcdSock);
    write_state(ch, 2, st);

    pollfd pfd{fd, POLLIN, 0};
    while (!g_stop) {
        int rc = poll(&pfd, 1, 1000);
        if (rc < 0) { if (errno == EINTR) continue; break; }
        if (rc == 0) continue;

        Reading r{};
        ssize_t n = recv(fd, &r, sizeof(r), 0);
        if (n != static_cast<ssize_t>(sizeof(r)) || r.type > 1) continue;

        double lat_us = (now_ns() - r.ts_ns) / 1000.0;
        st.messages++;
        st.lat_sum_us += lat_us;
        st.lat_max_us = std::max(st.lat_max_us, lat_us);

        Channel& c = ch[r.type];
        c.push(r.value);
        if (r.value > c.threshold) {
            if ((++c.alerts % 5) == 1)  // rate-limit log noise
                syslog(LOG_WARNING, "ALERT %s=%.2f > %.1f (total=%lu)",
                       c.name, r.value, c.threshold, c.alerts);
        }
        write_state(ch, 2, st);
    }

    syslog(LOG_INFO, "stopping after %lu messages", st.messages);
    close(fd);
    unlink(kProcdSock);
    closelog();
    return 0;
}
