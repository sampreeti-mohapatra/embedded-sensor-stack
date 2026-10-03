#pragma once
#include <cmath>
#include <random>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <cstdlib>
#include "protocol.hpp"

namespace stack {

// Hardware abstraction layer: swap SimulatedSensor for a real I2C/SPI
// driver later without touching the rest of the stack.
class ISensor {
public:
    virtual ~ISensor() = default;
    virtual SensorType  type() const = 0;
    virtual const char* name() const = 0;
    virtual double      read() = 0;
};

class SimulatedSensor : public ISensor {
public:
    SimulatedSensor(SensorType t, const char* name, double base, double amplitude,
                    double noise, unsigned seed)
        : type_(t), name_(name), base_(base), amp_(amplitude),
          noise_(0.0, noise), rng_(seed) {}

    SensorType  type() const override { return type_; }
    const char* name() const override { return name_; }

    double read() override {
        phase_ += 0.05;
        double v = base_ + amp_ * std::sin(phase_) + noise_(rng_);
        if (spike_(rng_) < 0.02) v += amp_ * 2.5;  // occasional anomaly
        return v;
    }

private:
    SensorType type_;
    const char* name_;
    double base_, amp_, phase_ = 0.0;
    std::normal_distribution<double> noise_;
    std::uniform_real_distribution<double> spike_{0.0, 1.0};
    std::mt19937 rng_;
};

// Reads a numeric value exposed by a Linux character device (for example
// /dev/sensor0). The driver returns one ASCII number followed by a newline.
class LinuxDeviceSensor : public ISensor {
public:
    LinuxDeviceSensor(SensorType t, std::string device_path, const char* sensor_name)
        : type_(t), path_(std::move(device_path)), name_(sensor_name) {}

    SensorType type() const override { return type_; }
    const char* name() const override { return name_.c_str(); }

    double read() override {
        std::ifstream input(path_);
        if (!input) throw std::runtime_error("cannot open sensor device: " + path_);
        std::string line;
        if (!std::getline(input, line))
            throw std::runtime_error("cannot read sensor device: " + path_);
        char* end = nullptr;
        const double value = std::strtod(line.c_str(), &end);
        if (end == line.c_str() || *end != '\0')
            throw std::runtime_error("invalid numeric data from sensor device: " + path_);
        return value;
    }

private:
    SensorType type_;
    std::string path_;
    std::string name_;
};

}  // namespace stack
