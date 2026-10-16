#include "wjs/platform/camera.hpp"

namespace wjs::platform::camera {

// A platform with no camera backend. Named by the platform.cmake of every
// platform that has none (today the three desktops): a webcam needs a capture
// API for each (Media Foundation, V4L2, AVFoundation, or Qt Multimedia, which the
// shared Qt tree does not have), and Mode A is an iPad feature of the MVP. The
// answer is always "unavailable", and the screen offers a typed-in board.

std::expected<std::vector<DeviceInfo>, CameraError> enumerate_devices() {
    return std::unexpected(CameraError::unavailable);
}

std::expected<void, CameraError> start(const DeviceInfo& /*device*/) {
    return std::unexpected(CameraError::unavailable);
}

void stop() {}

void open_permission_settings() {}

std::optional<scan::OwnedFrame> try_pop_latest_frame() {
    return std::nullopt;
}

bool is_capturing() noexcept {
    return false;
}

}  // namespace wjs::platform::camera
