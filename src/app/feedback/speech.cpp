#include "speech.hpp"

#include <utility>

#include "wjs/app/speech_backend.hpp"

namespace wjs::app {

class Speech::Impl {
public:
    SpeechBackend backend;
    int utterances = 0;
};

Speech::Speech(QObject* parent) : QObject(parent), impl_(std::make_unique<Impl>()) {}

Speech::~Speech() = default;

bool Speech::available() const {
    return impl_->backend.available();
}

int Speech::utterances() const {
    return impl_->utterances;
}

void Speech::say_word(const QString& word, double volume) {
    if (word.isEmpty()) {
        return;
    }
    ++impl_->utterances;
    impl_->backend.say(word, volume);
    emit spoken(word);
}

void Speech::say_letter_sequence(const QString&) {
    // The phoneme clips are the reviewer's recording job (docs/design/
    // phonics.md): there is no clip in the tree and the platform's voice must
    // not stand in for one, because letter names are not the sounds a child is
    // learning. When the clips exist this is where they play, before the word.
}

void Speech::say_text(const QString& text, double volume) {
    if (text.isEmpty()) {
        return;
    }
    ++impl_->utterances;
    impl_->backend.say(text, volume);
    emit spoken(text);
}

void Speech::stop() {
    impl_->backend.stop();
}

}  // namespace wjs::app
