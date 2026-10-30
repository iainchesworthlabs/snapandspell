// SPDX-License-Identifier: GPL-3.0-or-later
#include "sns/app/sound_backend.hpp"

#include <emscripten.h>

#include <QByteArray>
#include <QFile>
#include <QString>
#include <algorithm>
#include <string>
#include <string_view>

namespace {

// Whether the page has Web Audio. This is compiled into the program's script, not evaluated: the
// bundle's Content-Security-Policy allows no eval.
EM_JS(int, sns_web_audio_available, (), {
    return (typeof window !== 'undefined' && (window.AudioContext || window.webkitAudioContext)) ? 1 : 0;
});

// Decodes a WAV (from a copy of the bytes: the program's memory may move) the first time a name is
// asked for, and plays it at a volume; later calls play the decoded buffer. Safe where nothing can be
// played.
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

namespace sns::app {

SoundBackend::SoundBackend() = default;

SoundBackend::~SoundBackend() = default;

bool SoundBackend::available() const {
    return sns_web_audio_available() != 0;
}

void SoundBackend::play(std::string_view asset, double volume) {
    if (!available()) {
        return;
    }
    const QString stem = QString::fromUtf8(asset.data(), static_cast<qsizetype>(asset.size()));
    QFile file(QStringLiteral(":/sounds/") + stem + QStringLiteral(".wav"));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const QByteArray bytes = file.readAll();
    const std::string name(asset);
    sns_web_audio_play(name.c_str(), reinterpret_cast<const unsigned char*>(bytes.constData()),
                       static_cast<int>(bytes.size()), std::clamp(volume, 0.0, 1.0));
}

}  // namespace sns::app
