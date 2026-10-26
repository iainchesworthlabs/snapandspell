// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>

#include "scene.hpp"
#include "sns/scan/frame.hpp"
#include "sns/scan/image.hpp"

// A frame for the camera tests: one synthetic board from the scanner's corpus
// (libs/scan/tests/synth), copied into the owned frame a backend hands out. It is
// drawn from numbers and a seed, not from a camera, and it is the same picture
// the scanner's own tests read. Test support: nothing in libs or apps links it.

namespace sns::app::test {

[[nodiscard]] inline sns::scan::OwnedFrame synthetic_board_frame() {
    const sns::scan::synth::Spec spec{sns::scan::synth::Condition::upright,
                                      sns::scan::synth::Split::test, 4, 0};
    const sns::scan::synth::Rendered rendered = sns::scan::synth::render(spec);

    sns::scan::OwnedFrame frame;
    frame.width = static_cast<std::size_t>(rendered.image.width);
    frame.height = static_cast<std::size_t>(rendered.image.height);
    frame.stride = frame.width;
    frame.format = sns::scan::PixelFormat::gray8;
    frame.pixels = rendered.image.pixels;
    return frame;
}

}  // namespace sns::app::test
