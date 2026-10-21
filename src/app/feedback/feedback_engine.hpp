#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "sound_engine.hpp"
#include "speech.hpp"
#include "wjs/feedback.hpp"

namespace wjs::app {

// The one object the window's feedback seam talks to: it takes the cue the
// screens and models already emit, asks wjs::plan_cue() what the moment comes
// to, and carries it out through the sound engine, the platform's voice and
// the platform's haptics. The settings are the parent area's map, bound from
// Main.qml, so the Sound, Celebration sounds, Sound it out, Vibration and Shake
// rows are what decide it.
//
// Silence is a first-class answer: with Sound off nothing is played or said,
// with Celebration sounds off the cheer and the chime stop while the word is
// still spoken if Sound it out says so, and with Sound it out off the word is
// not spoken while the cheer still plays. A build without the Qt modules has
// the silent engines behind this, so every cue is still accepted and nothing
// crashes (docs/building.md, "The Qt Quick window").
class FeedbackEngine : public QObject {
    Q_OBJECT

    // The parent area's settings map, as ParentController::settings shows it.
    // The keys it reads are "sound", "celebrationSounds", "soundItOut" and
    // "vibration"; a missing or damaged value is the library's default.
    Q_PROPERTY(QVariantMap settings READ settings WRITE setSettings NOTIFY settingsChanged)

    // What the cue policy asked for since the program started: assets asked to
    // play, utterances asked of the voice, and haptic patterns asked of the
    // platform. Requests, not what was heard or felt, so the wiring can be
    // tested where there is no speaker and no accelerometer.
    Q_PROPERTY(int soundsPlayed READ soundsPlayed NOTIFY requestsChanged)
    Q_PROPERTY(int utterances READ utterances NOTIFY requestsChanged)
    Q_PROPERTY(int hapticsRequested READ hapticsRequested NOTIFY requestsChanged)

public:
    explicit FeedbackEngine(QObject* parent = nullptr);
    ~FeedbackEngine() override;

    [[nodiscard]] QVariantMap settings() const { return settings_map_; }
    void setSettings(const QVariantMap& settings);

    [[nodiscard]] int soundsPlayed() const;
    [[nodiscard]] int utterances() const noexcept;
    [[nodiscard]] int hapticsRequested() const noexcept { return haptics_requested_; }

    // One cue from the screens or a model: "rattle", "tile", "tile-landed",
    // "word", "tick", "chime", "cleared", "speak" or "tap". A kind that is not
    // one of those is ignored: the seam is not a place to be strict with a
    // child's game.
    Q_INVOKABLE void cue(const QString& kind, const QString& detail);

    // The engines behind the cues, for a test or a tool to put its own output on
    // (SoundEngine::setOutput, Speech::setOutput) so that nothing is heard.
    [[nodiscard]] SoundEngine& soundEngine() noexcept { return sound_; }
    [[nodiscard]] Speech& speech() noexcept { return speech_; }

signals:
    void settingsChanged();
    void requestsChanged();

private:
    SoundEngine sound_;
    Speech speech_;
    QVariantMap settings_map_;
    wjs::FeedbackSettings settings_;
    int haptics_requested_ = 0;
};

}  // namespace wjs::app
