#include "sound_engine.hpp"

#include <utility>

#include "wjs/app/sound_backend.hpp"

namespace wjs::app {
namespace {

[[nodiscard]] std::string utf8(const QString& text) {
    const QByteArray bytes = text.toUtf8();
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

}  // namespace

class SoundEngine::Impl {
public:
    SoundBackend backend;
    int plays = 0;
};

SoundEngine::SoundEngine(QObject* parent) : QObject(parent), impl_(std::make_unique<Impl>()) {}

SoundEngine::~SoundEngine() = default;

bool SoundEngine::available() const {
    return impl_->backend.available();
}

int SoundEngine::plays() const {
    return impl_->plays;
}

void SoundEngine::play(const QString& asset, double volume) {
    ++impl_->plays;
    impl_->backend.play(utf8(asset), volume);
    emit played(asset, volume);
}

}  // namespace wjs::app
