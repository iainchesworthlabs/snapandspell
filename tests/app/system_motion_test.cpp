#include <QAccessibilityHints>
#include <QByteArray>
#include <QGuiApplication>
#include <QMetaObject>
#include <QMetaProperty>
#include <QStyleHints>
#include <catch2/catch_test_macros.hpp>

#include "system_motion.hpp"

namespace {

using wjs::app::SystemMotion;

// The platform is read when the application is made, so it is chosen before it.
struct OffscreenPlatform {
    OffscreenPlatform() { qputenv("QT_QPA_PLATFORM", "offscreen"); }
};

// A GUI application on Qt's offscreen platform, which needs no display: the
// hints SystemMotion listens to belong to it.
class OffscreenApplication : private OffscreenPlatform {
public:
    OffscreenApplication() : application_(argc_, argv_) {}

    // The system's accessibility hints, which a test then tells of a change: the
    // platform that would say so is not there to ask. A signal is public in Qt 6; the
    // hints are const because nothing but the platform changes them.
    [[nodiscard]] QAccessibilityHints& hints() const {
        return *const_cast<QAccessibilityHints*>(QGuiApplication::styleHints()->accessibility());
    }

private:
    static inline int argc_ = 1;
    static inline char name_[] = "wjs_app_tests";
    static inline char* argv_[] = {name_, nullptr};

    QGuiApplication application_;
};

}  // namespace

TEST_CASE("the singleton starts with what the system's motion preference is",
          "[app][system][motion]") {
    OffscreenApplication app;
    SystemMotion motion;

    const bool asked = QGuiApplication::styleHints()->accessibility()->motionPreference() ==
                       Qt::MotionPreference::ReducedMotion;
    CHECK(motion.reduced() == asked);
}

TEST_CASE("a change of the system's motion preference is reported, once", "[app][system][motion]") {
    OffscreenApplication app;
    SystemMotion motion;
    app.hints().motionPreferenceChanged(Qt::MotionPreference::NoPreference);
    REQUIRE_FALSE(motion.reduced());

    int reports = 0;
    QObject::connect(&motion, &SystemMotion::reducedChanged, [&reports] { ++reports; });

    app.hints().motionPreferenceChanged(Qt::MotionPreference::NoPreference);
    CHECK_FALSE(motion.reduced());
    CHECK(reports == 0);  // no change, nothing to report

    app.hints().motionPreferenceChanged(Qt::MotionPreference::ReducedMotion);
    CHECK(motion.reduced());
    CHECK(reports == 1);

    app.hints().motionPreferenceChanged(Qt::MotionPreference::ReducedMotion);
    CHECK(motion.reduced());
    CHECK(reports == 1);

    app.hints().motionPreferenceChanged(Qt::MotionPreference::NoPreference);
    CHECK_FALSE(motion.reduced());
    CHECK(reports == 2);
}

TEST_CASE("the property is read by name and notifies, as QML needs", "[app][system][motion]") {
    OffscreenApplication app;
    SystemMotion motion;
    app.hints().motionPreferenceChanged(Qt::MotionPreference::NoPreference);

    const QMetaObject* meta = motion.metaObject();
    const QMetaProperty property = meta->property(meta->indexOfProperty("reduced"));
    REQUIRE(property.isValid());
    CHECK(property.hasNotifySignal());
    CHECK_FALSE(property.read(&motion).toBool());

    app.hints().motionPreferenceChanged(Qt::MotionPreference::ReducedMotion);
    CHECK(property.read(&motion).toBool());
}
