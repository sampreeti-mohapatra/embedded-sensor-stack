#pragma once
#include <cstdint>

namespace stack {

constexpr const char* kSockDir   = "/tmp/sensor-stack";
constexpr const char* kProcdSock = "/tmp/sensor-stack/procd.sock";
constexpr const char* kStateFile = "/tmp/sensor-stack/state.json";

enum class SensorType : std::uint8_t { Temperature = 0, Humidity = 1 };

// Wire format sent by sensord -> procd over a Unix datagram socket.
#pragma pack(push, 1)
struct Reading {
    std::uint64_t ts_ns;  // CLOCK_MONOTONIC at send time
    std::uint8_t  type;   // SensorType
    double        value;
};
#pragma pack(pop)

}  // namespace stack
