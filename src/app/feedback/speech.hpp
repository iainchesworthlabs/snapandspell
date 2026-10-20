#pragma once

#include <QObject>
#include <QString>
#include <memory>

namespace wjs::app {

// The device's own voice, over Qt TextToSpeech when the application was built
// with it and silently nothing when it was not. Whole words and sentences only:
// the isolated phonemes of the sound-it-out sequence are recordings the reviewer
// owns (docs/design/phonics.md), and say_letter_sequence() is the seam they
// will arrive through. It never captures audio: speech is output only.
//
// A build or a device with no voice engine answers available() false and every
// say() is a harmless request. The application's policy decides whether a word
// is said at all (Sound it out, and the master Sound switch); this class does
// not read the settings.
class Speech : public QObject {
    Q_OBJECT

    // Whether the platform has a text-to-speech engine at all.
    Q_PROPERTY(bool available READ available CONSTANT)
    // How many utterances have been asked for. A request, not a voice heard.
    Q_PROPERTY(int utterances READ utterances NOTIFY spoken)

public:
    explicit Speech(QObject* parent = nullptr);
    ~Speech() override;

    [[nodiscard]] bool available() const;
    [[nodiscard]] int utterances() const;

    // The whole word, through the platform's voice. `volume` is 0 to 1.
    void say_word(const QString& word, double volume = 1.0);

    // The sound-it-out sequence before a found word. The phoneme clips are the
    // reviewer's recording job and are not in the tree, so this is deliberately
    // silent today: the application calls it in full mode, and the day the
    // clips exist this is the one place that plays them. It must not fall back
    // to spelling the word: letter names are not its sounds.
    void say_letter_sequence(const QString& word);

    // A sentence or a picture's name, through the platform's voice.
    void say_text(const QString& text, double volume = 1.0);

    // Stops whatever is being said. Safe when nothing is.
    void stop();

signals:
    void spoken(const QString& text);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace wjs::app
