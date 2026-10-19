#include "wjs/scan/image.hpp"

#include <algorithm>
#include <cmath>

#include "scratch.hpp"
#include "wjs/scan/work.hpp"

namespace wjs::scan {

GrayImage::GrayImage(int w, int h, std::uint8_t fill)
    : width(w), height(h), pixels(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), fill) {
    detail::note_buffer();
}

void detail::luma_into(GrayImage& out, const Frame& frame) {
    const int width = static_cast<int>(frame.width);
    const int height = static_cast<int>(frame.height);
    detail::acquire_image(out, width, height);

    const std::size_t step = bytes_per_pixel(frame.format);
    const std::size_t row_bytes = frame.row_bytes();
    // The offsets of red, green and blue within a pixel.
    std::size_t red = 0;
    std::size_t green = 0;
    std::size_t blue = 0;
    switch (frame.format) {
        case PixelFormat::gray8:
            break;
        case PixelFormat::rgb8:
        case PixelFormat::rgba8:
            red = 0;
            green = 1;
            blue = 2;
            break;
        case PixelFormat::bgr8:
        case PixelFormat::bgra8:
            red = 2;
            green = 1;
            blue = 0;
            break;
    }

    for (std::size_t y = 0; y < frame.height; ++y) {
        const std::uint8_t* in = frame.pixels.data() + y * row_bytes;
        std::uint8_t* dest = out.pixels.data() + y * frame.width;
        if (frame.format == PixelFormat::gray8) {
            std::copy(in, in + frame.width, dest);
            continue;
        }
        for (std::size_t x = 0; x < frame.width; ++x, in += step) {
            const unsigned luma = 77U * in[red] + 150U * in[green] + 29U * in[blue] + 128U;
            dest[x] = static_cast<std::uint8_t>(luma >> 8U);
        }
    }
}

GrayImage luma_from_frame(const Frame& frame) {
    GrayImage out;
    detail::luma_into(out, frame);
    return out;
}

GrayImage shrink(const GrayImage& image, int max_long_side, int& factor) {
    const int long_side = std::max(image.width, image.height);
    factor = 1;
    if (max_long_side > 0 && long_side > max_long_side) {
        factor = (long_side + max_long_side - 1) / max_long_side;
    }
    if (factor == 1) {
        return image;
    }

    const int width = std::max(1, image.width / factor);
    const int height = std::max(1, image.height / factor);
    GrayImage out(width, height);
    for (int y = 0; y < height; ++y) {
        // A block is whole except where the picture is smaller than a block.
        const int rows = std::min(factor, image.height - y * factor);
        for (int x = 0; x < width; ++x) {
            const int columns = std::min(factor, image.width - x * factor);
            unsigned sum = 0;
            for (int dy = 0; dy < rows; ++dy) {
                const std::uint8_t* row =
                    image.pixels.data() + image.index(x * factor, y * factor + dy);
                for (int dx = 0; dx < columns; ++dx) {
                    sum += row[dx];
                }
            }
            const unsigned block = static_cast<unsigned>(rows) * static_cast<unsigned>(columns);
            out.at(x, y) = static_cast<std::uint8_t>((sum + block / 2U) / block);
        }
    }
    return out;
}

void detail::shrink_into(GrayImage& out, const GrayImage& image, int max_long_side,
                         int& factor) {
    const int long_side = std::max(image.width, image.height);
    factor = 1;
    if (max_long_side > 0 && long_side > max_long_side) {
        factor = (long_side + max_long_side - 1) / max_long_side;
    }
    if (factor == 1) {
        // The source already fits: the reader uses it as it is, no copy.
        return;
    }

    const int width = std::max(1, image.width / factor);
    const int height = std::max(1, image.height / factor);
    detail::acquire_image(out, width, height);
    for (int y = 0; y < height; ++y) {
        // A block is whole except where the picture is smaller than a block.
        const int rows = std::min(factor, image.height - y * factor);
        for (int x = 0; x < width; ++x) {
            const int columns = std::min(factor, image.width - x * factor);
            unsigned sum = 0;
            for (int dy = 0; dy < rows; ++dy) {
                const std::uint8_t* row =
                    image.pixels.data() + image.index(x * factor, y * factor + dy);
                for (int dx = 0; dx < columns; ++dx) {
                    sum += row[dx];
                }
            }
            const unsigned block = static_cast<unsigned>(rows) * static_cast<unsigned>(columns);
            out.at(x, y) = static_cast<std::uint8_t>((sum + block / 2U) / block);
        }
    }
}

double sample(const GrayImage& image, double x, double y) noexcept {
    const double fx = x - 0.5;
    const double fy = y - 0.5;
    const double floor_x = std::floor(fx);
    const double floor_y = std::floor(fy);
    const double tx = fx - floor_x;
    const double ty = fy - floor_y;
    const int x0 = std::clamp(static_cast<int>(floor_x), 0, image.width - 1);
    const int y0 = std::clamp(static_cast<int>(floor_y), 0, image.height - 1);
    const int x1 = std::clamp(static_cast<int>(floor_x) + 1, 0, image.width - 1);
    const int y1 = std::clamp(static_cast<int>(floor_y) + 1, 0, image.height - 1);
    const double top = image.at(x0, y0) * (1.0 - tx) + image.at(x1, y0) * tx;
    const double bottom = image.at(x0, y1) * (1.0 - tx) + image.at(x1, y1) * tx;
    return top * (1.0 - ty) + bottom * ty;
}

}  // namespace wjs::scan
