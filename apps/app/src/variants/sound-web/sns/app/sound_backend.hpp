// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string_view>

// The browser's sound backend: a WebAssembly build has no Qt Multimedia, so a cue is played through the
// page's Web Audio. It has the same header path and interface as the other sound variants, chosen by
// CMake (apps/app/src/feedback/sound_engine.cpp includes it). The WAV assets of assets/sounds are read
// from the program's own resources and handed to the browser to decode once; nothing is fetched, and
// nothing is recorded. A browser lets a page make sound only after the person has touched it, which a
// game's first press is; until then (and in a browser with no Web Audio) a cue plays nothing.

namespace sns::app {

class SoundBackend {
public:
    SoundBackend();
    ~SoundBackend();

    SoundBackend(const SoundBackend&) = delete;
    SoundBackend& operator=(const SoundBackend&) = delete;

    [[nodiscard]] bool available() const;
    // `asset` is a stem under assets/sounds; `volume` is 0 to 1.
    void play(std::string_view asset, double volume);
};

}  // namespace sns::app
