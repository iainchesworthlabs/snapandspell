// SPDX-License-Identifier: GPL-3.0-or-later
#include "media_probe.hpp"

#include <QList>
#include <QMediaFormat>
#include <QtLogging>

namespace sns::app::test {

bool mediaBackendAvailable() {
    static const bool available = [] {
        // Qt's default handler for the length of the probe: see media_probe.hpp.
        const QtMessageHandler previous = qInstallMessageHandler(nullptr);
        // The formats the backend can decode: none without a backend, and the platform integration
        // is created by the asking, so the question is a real one.
        const bool found = !QMediaFormat().supportedFileFormats(QMediaFormat::Decode).isEmpty();
        qInstallMessageHandler(previous);
        return found;
    }();
    return available;
}

}  // namespace sns::app::test
