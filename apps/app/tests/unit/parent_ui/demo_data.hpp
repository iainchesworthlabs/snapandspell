// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sns/phase_data.hpp"
#include "sns/store.hpp"

// TEST DATA for trying the parent area's screens. Every word, grapheme, sound
// and clip here was written for the tests and is not content for a child: the
// sounds are placeholders (`/t1/`), the words are ones the product brief itself
// names. It is not, and does not stand in for, a phase of the application; the
// real phase files have to be written and reviewed by a person qualified to do
// it (docs/design/phonics.md). The play in the store is made up.

namespace sns::app::demo {

// Two phases of test data: A of single letters and B, which adds "ai" and a
// grapheme with two sounds.
[[nodiscard]] sns::PhaseSet phases();

// A store with a few weeks of made-up play, as of `today`, in memory. The play
// is on phonics boards of phase B and on standard boards.
[[nodiscard]] sns::Store busy_store(sns::Day today);

// A store with nothing in it.
[[nodiscard]] sns::Store empty_store();

}  // namespace sns::app::demo
