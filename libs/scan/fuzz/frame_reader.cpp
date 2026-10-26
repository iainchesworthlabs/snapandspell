// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <span>

#include "sns/scan/frame.hpp"
#include "sns/scan/reader.hpp"

// A camera frame. The first five bytes say how to read the rest: the pixel format, the width and
// the height (each modulo 256 plus one, so a frame stays small enough to read in a few
// milliseconds), and a stride (0, or a number of bytes that may be too short for a row); the rest
// are the pixels. The reader answers a frame it cannot use with an error and never reads past the
// buffer (AddressSanitizer says if it does). A reading it gives has a tile for each cell of its
// grid.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    constexpr std::size_t header = 5;
    if (size < header) {
        return 0;
    }
    sns::scan::Frame frame;
    frame.format = static_cast<sns::scan::PixelFormat>(data[0] % 5);
    frame.width = static_cast<std::size_t>(data[1]) + 1;
    frame.height = static_cast<std::size_t>(data[2]) + 1;
    frame.stride = static_cast<std::size_t>(data[3]) | (static_cast<std::size_t>(data[4]) << 8U);
    frame.pixels = std::span<const std::uint8_t>(data + header, size - header);

    const auto reading = sns::scan::read_board(frame);
    if (reading && reading->tiles.size() != reading->rows * reading->columns) {
        std::abort();
    }
    return 0;
}
