#pragma once

// A PNG writer for the corpus tool, so that a person can look at what the
// generator draws. Uncompressed (stored deflate blocks), so it needs no library.
// A test tool: the application never writes a frame, and the build check
// scan.no_frame_files holds src/scan to it.

#include <filesystem>

#include "wjs/scan/image.hpp"

namespace wjs::scan::synth {

// Writes the image as an 8-bit grey PNG. False if the file could not be written.
[[nodiscard]] bool write_png(const std::filesystem::path& path, const GrayImage& image);

// Reads a binary grey PGM (P5, maxval 255), the format `[.recorded]` frames are in.
// An empty image if the file is not one.
[[nodiscard]] GrayImage read_pgm(const std::filesystem::path& path);

}  // namespace wjs::scan::synth
