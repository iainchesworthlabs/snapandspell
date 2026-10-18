#include <QString>
#include <QVariantMap>
#include <catch2/catch_test_macros.hpp>

#include "feedback_engine.hpp"

// The wiring behind Feedback.cue: the parent's settings map in, and the sound,
// speech and haptics requests out. The sounds themselves need a speaker and the
// voice needs a platform engine, so the cases below choose settings that make
// the policy silent: what is checked is what was asked for, which is the wiring
// docs/design/application.md's table describes. The cue-to-asset mapping and
// every gating rule are the pure policy's case (tests/unit/feedback_test.cpp).

namespace {

using wjs::app::FeedbackEngine;

[[nodiscard]] QVariantMap parent_settings(bool sound, bool vibration, const QString& celebration,
                                          const QString& sound_it_out) {
    QVariantMap settings;
    settings.insert(QStringLiteral("sound"), sound);
    settings.insert(QStringLiteral("vibration"), vibration);
    settings.insert(QStringLiteral("celebrationSounds"), celebration);
    settings.insert(QStringLiteral("soundItOut"), sound_it_out);
    // The rest of the parent's map is present and irrelevant; a settings map is
    // the whole of ParentController::settings, not four keys.
    settings.insert(QStringLiteral("help"), true);
    settings.insert(QStringLiteral("boardSize"), 3);
    return settings;
}

}  // namespace

TEST_CASE("the master Sound switch silences assets and speech and leaves vibration",
          "[app][feedback]") {
    FeedbackEngine engine;
    engine.setSettings(
        parent_settings(false, true, QStringLiteral("full"), QStringLiteral("full")));

    engine.cue(QStringLiteral("rattle"), QString());
    CHECK(engine.soundsPlayed() == 0);
    CHECK(engine.utterances() == 0);
    CHECK(engine.hapticsRequested() == 1);

    engine.cue(QStringLiteral("word"), QStringLiteral("sat"));
    CHECK(engine.soundsPlayed() == 0);
    CHECK(engine.utterances() == 0);
    CHECK(engine.hapticsRequested() == 2);

    engine.cue(QStringLiteral("speak"), QStringLiteral("box"));
    CHECK(engine.utterances() == 0);

    // Vibration is its own switch: off, the same cue asks for no tap.
    engine.setSettings(
        parent_settings(false, false, QStringLiteral("full"), QStringLiteral("full")));
    engine.cue(QStringLiteral("cleared"), QString());
    CHECK(engine.hapticsRequested() == 2);  // unchanged
    CHECK(engine.soundsPlayed() == 0);
}

TEST_CASE("Celebration sounds and Sound it out each stop their own half", "[app][feedback]") {
    FeedbackEngine engine;
    engine.setSettings(parent_settings(true, false, QStringLiteral("off"), QStringLiteral("off")));

    // Sound is on but the celebration is off and the word is not to be said:
    // this find asks for nothing at all, and neither does the new best or the
    // cleared board. No audio device is touched.
    engine.cue(QStringLiteral("word"), QStringLiteral("sat"));
    engine.cue(QStringLiteral("chime"), QString());
    engine.cue(QStringLiteral("cleared"), QString());
    CHECK(engine.soundsPlayed() == 0);
    CHECK(engine.utterances() == 0);
    CHECK(engine.hapticsRequested() == 0);

    // Master Sound off silences the word even while Sound it out says full.
    engine.setSettings(
        parent_settings(false, false, QStringLiteral("off"), QStringLiteral("full")));
    engine.cue(QStringLiteral("word"), QStringLiteral("sat"));
    CHECK(engine.utterances() == 0);
}

TEST_CASE("a cue kind the design does not have is ignored", "[app][feedback]") {
    FeedbackEngine engine;
    engine.setSettings(
        parent_settings(false, true, QStringLiteral("full"), QStringLiteral("full")));
    const int haptics = engine.hapticsRequested();

    engine.cue(QStringLiteral("not-a-cue"), QStringLiteral("anything"));
    engine.cue(QString(), QString());

    CHECK(engine.soundsPlayed() == 0);
    CHECK(engine.utterances() == 0);
    CHECK(engine.hapticsRequested() == haptics);
}

TEST_CASE("the engine starts with the library's defaults", "[app][feedback]") {
    // Before Main.qml binds the parent's map: sound on, full celebrations, full
    // sound-it-out, vibration on. The values are read from the map the engine
    // would use, so a settings row cannot be missing a default.
    const FeedbackEngine engine;
    const QVariantMap settings = engine.settings();
    CHECK(settings.value(QStringLiteral("sound")).toBool());
    CHECK(settings.value(QStringLiteral("vibration")).toBool());
    CHECK(settings.value(QStringLiteral("celebrationSounds")).toString() == QStringLiteral("full"));
    CHECK(settings.value(QStringLiteral("soundItOut")).toString() == QStringLiteral("full"));
}
