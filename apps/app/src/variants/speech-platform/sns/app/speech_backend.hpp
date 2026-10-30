// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>

// The platform's speech backend: no Qt TextToSpeech in the build, so a word goes to the platform's own
// voice where it has one (the browser's speech synthesis) and every request is harmless where it has
// none. It has the same header path and interface as the Qt TextToSpeech variant, chosen by CMake
// (apps/app/src/feedback/speech.cpp includes it).

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
