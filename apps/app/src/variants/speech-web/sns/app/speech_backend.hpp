// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>

// The browser's voice: a WebAssembly build has no Qt TextToSpeech, so a word is said through the
// browser's speech synthesis. It has the same header path and interface as the other speech variants,
// chosen by CMake (apps/app/src/feedback/speech.cpp includes it).
//
// Only a voice that runs on the device is used. Some browsers list voices that send the text to their
// maker's server to be spoken; the program sends nothing anywhere (docs/web.md), so a browser with no
// local English voice has no voice here, and every request is a harmless nothing.

namespace sns::app {

class SpeechBackend {
public:
    SpeechBackend();
    ~SpeechBackend();

    SpeechBackend(const SpeechBackend&) = delete;
    SpeechBackend& operator=(const SpeechBackend&) = delete;

    [[nodiscard]] bool available() const;
    void say(const QString& text, double volume);
    void stop();
};

}  // namespace sns::app
