// SPDX-License-Identifier: GPL-3.0-or-later
#include <emscripten.h>

#include <algorithm>
#include <string>

#include "sns/platform/speech_output.hpp"

// The JavaScript in EM_JS is the browser's, not C++: the formatter must not touch it.
// clang-format off
namespace {

// Whether the browser has a local English voice. A browser loads its voice list after the page starts,
// so the list is asked for here and again when it announces a change. A voice whose `localService` is
// false is spoken by a server and is never used: the program sends nothing anywhere.
EM_JS(int, sns_web_speech_available, (), {
    if (typeof window === 'undefined' || !window.speechSynthesis) {
        return 0;
    }
    const sns = (Module.snsWebSpeech = Module.snsWebSpeech || {voice: null, listening: false});
    const pick = () => {
        const voices = window.speechSynthesis.getVoices();
        sns.voice = voices.find((v) => v.localService && v.lang && v.lang.toLowerCase().startsWith('en')) || null;
    };
    pick();
    if (!sns.listening) {
        sns.listening = true;
        window.speechSynthesis.addEventListener('voiceschanged', pick);
    }
    return sns.voice ? 1 : 0;
});

EM_JS(void, sns_web_speech_say, (const char* text, double volume), {
    const sns = Module.snsWebSpeech;
    if (!sns || !sns.voice) {
        return;
    }
    const utterance = new SpeechSynthesisUtterance(UTF8ToString(text));
    utterance.voice = sns.voice;
    utterance.lang = sns.voice.lang;
    utterance.volume = volume;
    // A little slower than natural: easier for a beginning reader to follow.
    utterance.rate = 0.85;
    window.speechSynthesis.cancel();
    window.speechSynthesis.speak(utterance);
});

EM_JS(void, sns_web_speech_stop, (), {
    if (typeof window !== 'undefined' && window.speechSynthesis) {
        window.speechSynthesis.cancel();
    }
});

}  // namespace
// clang-format on

namespace sns::platform {

bool speech_output_available() {
    return sns_web_speech_available() != 0;
}

void speak(std::string_view utf8, double volume) {
    if (utf8.empty() || !speech_output_available()) {
        return;
    }
    const std::string text(utf8);
    sns_web_speech_say(text.c_str(), std::clamp(volume, 0.0, 1.0));
}

void stop_speaking() {
    sns_web_speech_stop();
}

}  // namespace sns::platform
