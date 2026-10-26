// SPDX-License-Identifier: GPL-3.0-or-later
#include "sns/app/motion_backend.hpp"

#include <utility>

// Nothing to do: this variant is compiled when Qt Sensors is not in the kit.
// available() and start() are false, no sample is ever sent, and the drag that
// stands in for a shake stays (SoloScreen.qml).

namespace sns::app {

class MotionBackend::Impl {
public:
    explicit Impl(Sink sink) : sink_(std::move(sink)) {}

    [[nodiscard]] bool available() const { return false; }
    bool start() { return false; }
    void stop() {}

private:
    [[maybe_unused]] Sink sink_;
};

MotionBackend::MotionBackend(Sink sink) : impl_(std::make_unique<Impl>(std::move(sink))) {}

MotionBackend::~MotionBackend() = default;

bool MotionBackend::available() {
    return impl_->available();
}

bool MotionBackend::start() {
    return impl_->start();
}

void MotionBackend::stop() {
    impl_->stop();
}

}  // namespace sns::app
