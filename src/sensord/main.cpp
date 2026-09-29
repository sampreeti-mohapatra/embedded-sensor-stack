// sensord: reads (simulated) sensors and sends readings to procd.
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>

#include "common/hal.hpp"
#include "common/util.hpp"

using namespace stack;

int main(int argc, char** argv) {
    int interval_ms = 500;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--interval-ms") && i + 1 < argc) {
            interval_ms = std::atoi(argv[++i]);
        } else {
            std::fprintf(stderr, "usage: %s [--interval-ms N]\n", argv[0]);
            return 2;
        }
    }
    if (interval_ms <= 0) interval_ms = 500;

    openlog("sensord", LOG_PID, LOG_DAEMON);
    install_signal_handlers();

    std::vector<std::unique_ptr<ISensor>> sensors;
    sensors.emplace_back(std::make_unique<SimulatedSensor>(
        SensorType::Temperature, "sim-temp", 28.0, 5.0, 0.4, 1));
    sensors.emplace_back(std::make_unique<SimulatedSensor>(
        SensorType::Humidity, "sim-hum", 60.0, 15.0, 1.0, 2));

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) { syslog(LOG_ERR, "socket: %s", std::strerror(errno)); return 1; }

    sockaddr_un dst{};
    dst.sun_family = AF_UNIX;
    std::strncpy(dst.sun_path, kProcdSock, sizeof(dst.sun_path) - 1);

    syslog(LOG_INFO, "started, interval=%dms", interval_ms);
    unsigned long dropped = 0;

    while (!g_stop) {
        for (auto& s : sensors) {
            Reading r{now_ns(), static_cast<std::uint8_t>(s->type()), s->read()};
            ssize_t n = sendto(fd, &r, sizeof(r), 0,
                               reinterpret_cast<sockaddr*>(&dst), sizeof(dst));
            if (n < 0 && (++dropped % 20) == 1)
                syslog(LOG_WARNING, "procd unavailable (%s), dropped=%lu",
                       std::strerror(errno), dropped);
        }
        sleep_ms(interval_ms);
    }

    syslog(LOG_INFO, "stopping");
    close(fd);
    closelog();
    return 0;
}
