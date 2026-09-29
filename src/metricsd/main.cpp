// metricsd: tiny HTTP server exposing /health, /metrics (from /proc) and /state.
#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <syslog.h>
#include <unistd.h>

#include "common/protocol.hpp"
#include "common/util.hpp"

using namespace stack;

namespace {

std::string read_file(const char* path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string system_metrics() {
    std::ostringstream o;
    double up = 0, idle = 0;
    { std::ifstream f("/proc/uptime"); f >> up >> idle; }
    double l1 = 0, l5 = 0, l15 = 0;
    { std::ifstream f("/proc/loadavg"); f >> l1 >> l5 >> l15; }
    long mem_total = 0, mem_avail = 0;
    {
        std::ifstream f("/proc/meminfo");
        std::string key, unit;
        long val;
        while (f >> key >> val >> unit) {
            if (key == "MemTotal:") mem_total = val;
            if (key == "MemAvailable:") mem_avail = val;
        }
    }
    o << "uptime_seconds " << up << "\n"
      << "load1 " << l1 << "\nload5 " << l5 << "\nload15 " << l15 << "\n"
      << "mem_total_kb " << mem_total << "\n"
      << "mem_available_kb " << mem_avail << "\n";
    return o.str();
}

void respond(int cfd, int code, const char* ctype, const std::string& body) {
    std::ostringstream o;
    o << "HTTP/1.1 " << code << (code == 200 ? " OK" : " Not Found") << "\r\n"
      << "Content-Type: " << ctype << "\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Connection: close\r\n\r\n" << body;
    std::string s = o.str();
    (void)!write(cfd, s.data(), s.size());
}

}  // namespace

int main(int argc, char** argv) {
    int port = 8080;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--port") && i + 1 < argc) port = std::atoi(argv[++i]);
        else { std::fprintf(stderr, "usage: %s [--port N]\n", argv[0]); return 2; }
    }

    openlog("metricsd", LOG_PID, LOG_DAEMON);
    install_signal_handlers();

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(static_cast<uint16_t>(port));
    if (bind(sfd, reinterpret_cast<sockaddr*>(&a), sizeof(a)) < 0 || listen(sfd, 8) < 0) {
        syslog(LOG_ERR, "bind/listen on %d: %s", port, std::strerror(errno));
        return 1;
    }
    syslog(LOG_INFO, "listening on :%d", port);

    pollfd pfd{sfd, POLLIN, 0};
    while (!g_stop) {
        int rc = poll(&pfd, 1, 1000);
        if (rc < 0) { if (errno == EINTR) continue; break; }
        if (rc == 0) continue;

        int cfd = accept(sfd, nullptr, nullptr);
        if (cfd < 0) continue;
        timeval tv{1, 0};
        setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        char buf[1024];
        ssize_t n = read(cfd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            char method[8] = {0}, path[128] = {0};
            std::sscanf(buf, "%7s %127s", method, path);
            if (!std::strcmp(path, "/health"))       respond(cfd, 200, "text/plain", "ok\n");
            else if (!std::strcmp(path, "/metrics")) respond(cfd, 200, "text/plain", system_metrics());
            else if (!std::strcmp(path, "/state")) {
                std::string s = read_file(kStateFile);
                respond(cfd, 200, "application/json", s.empty() ? "{}\n" : s);
            } else respond(cfd, 404, "text/plain", "not found\n");
        }
        close(cfd);
    }

    syslog(LOG_INFO, "stopping");
    close(sfd);
    closelog();
    return 0;
}
