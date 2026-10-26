// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <expected>

#include "sns/scan/frame.hpp"
#include "sns/scan/reading.hpp"

namespace sns::scan {

// Reads a board from one frame: finds it, draws it square, counts its cells and
// reads each cell's one glyph, whatever its turn. Portable and Qt-free; the
// platform's board_reader (libs/platforms/interfaces/sns/platform/board_reader.hpp)
// is how the application asks for it, and may replace parts of it on a platform
// that has something better.
//
// A pure function of the frame and the options, with the frame only read. Nothing
// is written anywhere. A frame that is not valid() and options that are not
// valid() are answered with an error, never read. The only way out of it that is
// not an error is std::bad_alloc, from a picture too large to hold.
[[nodiscard]] std::expected<BoardReading, ReadError> read_board(const Frame& frame,
                                                                const ReadOptions& options = {});

}  // namespace sns::scan
