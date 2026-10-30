// SPDX-License-Identifier: GPL-3.0-or-later
#include "sns/app/speech_backend.hpp"

#include <QByteArray>
#include <cstddef>
#include <string_view>

#include "sns/platform/speech_output.hpp"

// Compiled when Qt TextToSpeech is not in the kit. A word goes to the platform's own voice
// (sns/platform/speech_output.hpp: the browser's speech synthesis, through a voice that runs on the
// device), and where the platform has none the request is a harmless nothing: the word is also on
// screen, so nothing is lost.

namespace sns::app {

SpeechBackend::SpeechBackend() = default;

SpeechBackend::~SpeechBackend() = default;

bool SpeechBackend::available() const {
    return sns::platform::speech_output_available();
}

void SpeechBackend::say(const QString& text, double volume) {
    if (text.isEmpty()) {
        return;
    }
    const QByteArray utf8 = text.toUtf8();
    sns::platform::speak(std::string_view(utf8.constData(), static_cast<std::size_t>(utf8.size())), volume);
}

void SpeechBackend::stop() {
    sns::platform::stop_speaking();
}

}  // namespace sns::app
