// SPDX-License-Identifier: GPL-3.0-or-later
#include "motion_source.hpp"

#include <functional>
#include <utility>

#include "sns/app/motion_backend.hpp"

namespace sns::app {

class MotionSource::Impl {
public:
    Impl()
        : backend([this](const sns::DeviceMotion& sample) {
              if (sink) {
                  sink(processor.push(sample));
              }
          }) {}

    sns::MotionProcessor processor;
    MotionBackend backend;
    std::function<void(const sns::MotionOutput&)> sink;
};

MotionSource::MotionSource(QObject* parent) : QObject(parent), impl_(std::make_unique<Impl>()) {
    impl_->sink = [this](const sns::MotionOutput& output) {
        if (!available_) {
            // The first real sample is what makes the sensors "available": a
            // registered type with no hardware never reaches this.
            available_ = true;
            emit availableChanged();
        }
        emit sample(output.ax, output.ay, output.az, output.wx, output.wy, output.wz);
        if (output.shake && shake_enabled_) {
            emit shakeDetected();
        }
    };
}

void MotionSource::accept(const sns::DeviceMotion& motion) {
    if (impl_->sink) {
        impl_->sink(impl_->processor.push(motion));
    }
}

MotionSource::~MotionSource() {
    impl_->backend.stop();
}

bool MotionSource::available() const {
    return available_;
}

void MotionSource::setActive(bool active) {
    if (active_ == active) {
        return;
    }
    active_ = active;
    if (active_) {
        // A sensor that was stopped and started is a new trace: a shake from
        // before the screen was left is not one now.
        impl_->processor.reset();
        impl_->backend.start();
    } else {
        impl_->backend.stop();
    }
    emit activeChanged();
}

void MotionSource::setShakeEnabled(bool enabled) {
    if (shake_enabled_ == enabled) {
        return;
    }
    shake_enabled_ = enabled;
    emit shakeEnabledChanged();
}

}  // namespace sns::app
