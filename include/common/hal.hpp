#pragma once
#include <cmath>
#include <random>
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

}  // namespace stack
