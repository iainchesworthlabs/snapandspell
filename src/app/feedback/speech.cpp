#include "speech.hpp"

#include <utility>

#include "wjs/profiling.hpp"

#include "wjs/app/speech_backend.hpp"

namespace wjs::app {

class Speech::Impl {
public:
    SpeechBackend backend;
    Speech::Output output;
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

void Speech::setOutput(Output output) {
    impl_->output = std::move(output);
}

bool Speech::say(const QString& text, double volume) {
    if (text.isEmpty()) {
        return false;
    }
    // The first utterance makes the voice engine, which is the long one.
    const auto zone = wjs::profiling::zone<"speech.say">();
    ++impl_->utterances;
    if (impl_->output) {
        impl_->output(text, volume);
    } else {
        impl_->backend.say(text, volume);
    }
    emit spoken(text);
    return true;
}

bool Speech::say_word(const QString& word, double volume) {
    return say(word, volume);
}

void Speech::say_letter_sequence(const QString&) {
    // The phoneme clips are the reviewer's recording job (docs/design/
    // phonics.md): there is no clip in the tree and the platform's voice must
    // not stand in for one, because letter names are not the sounds a child is
    // learning. When the clips exist this is where they play, before the word.
}

bool Speech::say_text(const QString& text, double volume) {
    return say(text, volume);
}

void Speech::stop() {
    impl_->backend.stop();
}

}  // namespace wjs::app
