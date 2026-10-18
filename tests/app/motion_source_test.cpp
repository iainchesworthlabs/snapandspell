#include <QByteArray>
#include <QGuiApplication>
#include <catch2/catch_test_macros.hpp>

#include "motion_source.hpp"

// The MotionSource the window binds to: on the desktops there is no
// accelerometer, so the source is unavailable, starting it is harmless, no
// sample arrives, and the drag stands in (SoloScreen.qml). The processing
// itself is the pure library's case (tests/unit/motion_test.cpp), so no
// hardware is needed here either.

namespace {

struct OffscreenPlatform {
    OffscreenPlatform() { qputenv("QT_QPA_PLATFORM", "offscreen"); }
};

// Qt Sensors asks the platform for its plugins, which wants an application.
class OffscreenApplication : private OffscreenPlatform {
public:
    OffscreenApplication() : application_(argc_, argv_) {}

private:
    static inline int argc_ = 1;
    static inline char name_[] = "wjs_app_tests";
    static inline char* argv_[] = {name_, nullptr};

    QGuiApplication application_;
};

}  // namespace

TEST_CASE("a desktop motion source has no accelerometer and stops safely", "[app][motion]") {
    [[maybe_unused]] OffscreenApplication app;
    wjs::app::MotionSource motion;

    CHECK_FALSE(motion.available());
    CHECK_FALSE(motion.active());

    int samples = 0;
    int shakes = 0;
    QObject::connect(&motion, &wjs::app::MotionSource::sample,
                     [&samples](qreal, qreal, qreal, qreal, qreal, qreal) { ++samples; });
    QObject::connect(&motion, &wjs::app::MotionSource::shakeDetected, [&shakes] { ++shakes; });

    motion.setShakeEnabled(false);
    CHECK_FALSE(motion.shakeEnabled());
    motion.setActive(true);
    CHECK(motion.active());
    motion.setActive(false);
    CHECK_FALSE(motion.active());

    CHECK(samples == 0);
    CHECK(shakes == 0);
}
