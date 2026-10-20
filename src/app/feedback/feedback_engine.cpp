#include "feedback_engine.hpp"

#include <QByteArray>
#include <QString>
#include <string>
#include <string_view>
#include <utility>

#include "wjs/platform/haptics.hpp"
#include "wjs/settings.hpp"

namespace wjs::app {
namespace {

[[nodiscard]] std::string utf8(const QString& text) {
    const QByteArray bytes = text.toUtf8();
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

[[nodiscard]] QString from_utf8(std::string_view text) {
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

// The map's value for `key`, or `fallback` when the parent area has not filled
// it or it is not the type the screens write.
template <typename Value>
[[nodiscard]] Value value_or(const QVariantMap& map, const QString& key, Value fallback) {
    const QVariant value = map.value(key);
    if (!value.isValid()) {
        return fallback;
    }
    return value.value<Value>();
}

[[nodiscard]] wjs::FeedbackSettings policy_settings(const QVariantMap& map) {
    wjs::FeedbackSettings settings;
    settings.sound = value_or<bool>(map, QStringLiteral("sound"), settings.sound);
    settings.vibration = value_or<bool>(map, QStringLiteral("vibration"), settings.vibration);

    const auto celebration = wjs::celebration_sounds_from_string(
        utf8(value_or<QString>(map, QStringLiteral("celebrationSounds"), QStringLiteral("full"))));
    if (celebration) {
        settings.celebration_sounds = *celebration;
    }
    const auto sound_it_out = wjs::sound_it_out_from_string(
        utf8(value_or<QString>(map, QStringLiteral("soundItOut"), QStringLiteral("full"))));
    if (sound_it_out) {
        settings.sound_it_out = *sound_it_out;
    }
    return settings;
}

[[nodiscard]] wjs::platform::HapticPattern platform_pattern(wjs::HapticCue cue) {
    switch (cue) {
        case wjs::HapticCue::light:
            return wjs::platform::HapticPattern::light;
        case wjs::HapticCue::medium:
            return wjs::platform::HapticPattern::medium;
        case wjs::HapticCue::double_tap:
            return wjs::platform::HapticPattern::double_tap;
        case wjs::HapticCue::none:
            break;
    }
    return wjs::platform::HapticPattern::light;
}

}  // namespace

FeedbackEngine::FeedbackEngine(QObject* parent)
    : QObject(parent),
      settings_map_{{QStringLiteral("sound"), true},
                    {QStringLiteral("celebrationSounds"), QStringLiteral("full")},
                    {QStringLiteral("soundItOut"), QStringLiteral("full")},
                    {QStringLiteral("vibration"), true}} {}

FeedbackEngine::~FeedbackEngine() = default;

void FeedbackEngine::setSettings(const QVariantMap& settings) {
    if (settings == settings_map_) {
        return;
    }
    settings_map_ = settings;
    settings_ = policy_settings(settings_map_);
    emit settingsChanged();
}

int FeedbackEngine::soundsPlayed() const {
    return sound_.plays();
}

void FeedbackEngine::cue(const QString& kind, const QString& detail) {
    const auto parsed = wjs::feedback_cue_from_string(utf8(kind));
    if (!parsed) {
        return;
    }

    const wjs::CuePlan plan = wjs::plan_cue(*parsed, utf8(detail), settings_);
    bool asked = false;

    if (plan.sound) {
        sound_.play(from_utf8(plan.asset), plan.volume);
        asked = true;
    }
    // The sound-it-out sequence is the seam for the reviewer's phoneme clips:
    // it is silent today, and called before the word so that the day the clips
    // exist the word still comes last (docs/design/phonics.md).
    if (plan.speak_sequence) {
        speech_.say_letter_sequence(detail);
        asked = true;
    }
    if (plan.speak_word) {
        speech_.say_word(detail, plan.volume);
        ++utterances_;
        asked = true;
    } else if (plan.speak_text) {
        speech_.say_text(detail, plan.volume);
        ++utterances_;
        asked = true;
    }
    if (plan.haptic != wjs::HapticCue::none) {
        ++haptics_requested_;
        // The desktops answer false; a tablet will answer with its own engine.
        // The answer is deliberately not read: every haptic is paired with the
        // sound and the picture that were already asked for.
        static_cast<void>(wjs::platform::vibrate(platform_pattern(plan.haptic)));
        asked = true;
    }

    if (asked) {
        emit requestsChanged();
    }
}

}  // namespace wjs::app
