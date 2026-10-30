// SPDX-License-Identifier: GPL-3.0-or-later
#include <emscripten.h>

#include <algorithm>
#include <string>

#include "sns/platform/audio_output.hpp"

// The JavaScript in EM_JS is the browser's, not C++: the formatter must not touch it.
// clang-format off
namespace {

// Whether the page has Web Audio. This is compiled into the program's script, not evaluated: the
// bundle's Content-Security-Policy allows no eval.
EM_JS(int, sns_web_audio_available, (), {
    return (typeof window !== 'undefined' && (window.AudioContext || window.webkitAudioContext)) ? 1 : 0;
});

// Decodes a WAV (from a copy of the bytes: the program's memory may move) the first time a name is
// asked for, and plays it at a volume; later calls play the decoded buffer. A browser lets a page make
// sound only after the person has touched it, which a game's first press is: the context is resumed
// on the call, and a cue before any touch is the browser's to refuse.
EM_JS(void, sns_web_audio_play, (const char* name, const unsigned char* data, int size, double volume), {
    const key = UTF8ToString(name);
    const sns = (Module.snsWebAudio = Module.snsWebAudio || {context: null, buffers: {}, pending: {}});
    const Context = window.AudioContext || window.webkitAudioContext;
    if (!Context) {
        return;
    }
    if (!sns.context) {
        sns.context = new Context();
    }
    const context = sns.context;
    if (context.state === 'suspended') {
        context.resume();
    }
    const start = (buffer) => {
        const source = context.createBufferSource();
        const gain = context.createGain();
        gain.gain.value = volume;
        source.buffer = buffer;
        source.connect(gain);
        gain.connect(context.destination);
        source.start();
    };
    if (sns.buffers[key]) {
        start(sns.buffers[key]);
        return;
    }
    if (sns.pending[key] || size <= 0) {
        return;
    }
    sns.pending[key] = true;
    const copy = HEAPU8.slice(data, data + size).buffer;
    context.decodeAudioData(copy).then((buffer) => {
        sns.buffers[key] = buffer;
        delete sns.pending[key];
        start(buffer);
    }, () => {
        delete sns.pending[key];
    });
});

}  // namespace
// clang-format on

namespace sns::platform {

bool audio_output_available() {
    return sns_web_audio_available() != 0;
}

void play_wav(std::string_view name, std::span<const std::byte> wav, double volume) {
    if (wav.empty() || !audio_output_available()) {
        return;
    }
    const std::string key(name);
    sns_web_audio_play(key.c_str(), reinterpret_cast<const unsigned char*>(wav.data()),
                       static_cast<int>(wav.size()), std::clamp(volume, 0.0, 1.0));
}

}  // namespace sns::platform
