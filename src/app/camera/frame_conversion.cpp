#include "frame_conversion.hpp"

#include <QImage>
#include <chrono>
#include <cstdint>
#include <cstring>

namespace wjs::app {
namespace {

[[nodiscard]] std::int64_t now_ms() noexcept {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

}  // namespace

std::optional<wjs::scan::OwnedFrame> frame_from_image(const QImage& image) {
    if (image.isNull() || image.width() <= 0 || image.height() <= 0) {
        return std::nullopt;
    }
    const QImage rgb = image.convertToFormat(QImage::Format_RGB888);
    if (rgb.isNull() || rgb.width() <= 0 || rgb.height() <= 0) {
        return std::nullopt;
    }

    wjs::scan::OwnedFrame frame;
    frame.width = static_cast<std::size_t>(rgb.width());
    frame.height = static_cast<std::size_t>(rgb.height());
    frame.stride = frame.width * 3;
    frame.format = wjs::scan::PixelFormat::rgb8;
    frame.pixels.resize(frame.height * frame.stride);

    // QImage pads each row to a multiple of four bytes, so the rows are copied
    // one by one into the packed buffer the scanner expects.
    for (int y = 0; y < rgb.height(); ++y) {
        std::memcpy(frame.pixels.data() + static_cast<std::size_t>(y) * frame.stride,
                    rgb.constScanLine(y), frame.stride);
    }
    frame.t_ms = now_ms();
    return frame;
}

}  // namespace wjs::app
