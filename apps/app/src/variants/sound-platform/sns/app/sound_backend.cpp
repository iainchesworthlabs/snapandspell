// SPDX-License-Identifier: GPL-3.0-or-later
#include "sns/app/sound_backend.hpp"

#include <QByteArray>
#include <QFile>
#include <QString>
#include <cstddef>
#include <span>

#include "sns/platform/audio_output.hpp"

// Compiled when Qt Multimedia is not in the kit. The cue goes to the platform's own output
// (sns/platform/audio_output.hpp: Web Audio in the browser), and where the platform has none
// nothing is played: the engine still counts the cue as a request, so the wiring is the same and a
// build without sound is still a working game.

namespace sns::app {

SoundBackend::SoundBackend() = default;

SoundBackend::~SoundBackend() = default;

bool SoundBackend::available() const {
    return sns::platform::audio_output_available();
}

void SoundBackend::play(std::string_view asset, double volume) {
    if (!available()) {
        return;
    }
    // The same resource the Multimedia variant plays, by the same stem.
    const QString stem = QString::fromUtf8(asset.data(), static_cast<qsizetype>(asset.size()));
    QFile file(QStringLiteral(":/sounds/") + stem + QStringLiteral(".wav"));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const QByteArray bytes = file.readAll();
    sns::platform::play_wav(
        asset,
        std::span<const std::byte>(reinterpret_cast<const std::byte*>(bytes.constData()),
                                   static_cast<std::size_t>(bytes.size())),
        volume);
}

}  // namespace sns::app
