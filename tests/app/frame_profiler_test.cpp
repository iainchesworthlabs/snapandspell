#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

#include "frame_profiler.hpp"

// The frame clock (src/app/frame_profiler.cpp) on a window that is never shown:
// the signals it listens to are public, so a test emits them as the scene graph
// does, which makes the frame, the thread it is on and the moment it ends the
// test's. What this holds: what each signal adds to the report, and that the render
// thread may be in the middle of a slot when the profiler is destroyed.

namespace {

using wjs::app::FrameProfiler;

// The platform is read when the application is made, so it is chosen before it.
struct OffscreenPlatform {
    OffscreenPlatform() { qputenv("QT_QPA_PLATFORM", "offscreen"); }
};

// A GUI application on Qt's offscreen platform, which needs no display.
class OffscreenApplication : private OffscreenPlatform {
public:
    OffscreenApplication() : application_(argc_, argv_) {}

private:
    static inline int argc_ = 1;
    static inline char name_[] = "wjs_frame_profiler_tests";
    static inline char* argv_[] = {name_, nullptr};

    QGuiApplication application_;
};

}  // namespace

TEST_CASE("each of the window's signals adds to its own part of the report", "[app][frame]") {
    OffscreenApplication app;
    QQuickWindow window;
    FrameProfiler profiler(&window);

    // The render thread's frame, its synchronisation and its recording of the pass.
    window.beforeFrameBegin();
    window.beforeSynchronizing();
    window.afterSynchronizing();
    window.beforeRenderPassRecording();
    window.afterRenderPassRecording();
    window.afterFrameEnd();
    window.frameSwapped();
    window.frameSwapped();

    // The GUI thread's: the end of a frame's animations and polish.
    window.afterAnimating();

    const FrameProfiler::Report report = profiler.report();
    CHECK(report.render_frames == 1);
    CHECK(report.sync_frames == 1);
    CHECK(report.record_frames == 1);
    CHECK(report.animate_frames == 1);
    // Two swaps are one interval: the first has nothing before it.
    CHECK(report.frames == 1);
    CHECK(profiler.first_frame_seen());
    CHECK(report.refresh_hz == window.screen()->refreshRate());
    CHECK(report.refresh_hz > 0.0);

    const QString text = profiler.report_text();
    CHECK(text.contains(QStringLiteral("refresh_hz=")));
    CHECK(text.contains(QStringLiteral("render.record_ms:")));
    CHECK(text.contains(QStringLiteral("ui.animate_ms:")));
}

TEST_CASE("the GUI thread's frame is the time it was busy, without the wait for the render thread",
          "[app][frame]") {
    OffscreenApplication app;
    QQuickWindow window;
    FrameProfiler profiler(&window);
    QAbstractEventDispatcher* loop = QAbstractEventDispatcher::instance();
    REQUIRE(loop != nullptr);
    using namespace std::chrono_literals;

    // A wakeup: busy for a while, then the animations and polish are done.
    loop->awake();
    std::this_thread::sleep_for(15ms);
    window.afterAnimating();
    // The render thread synchronises while the GUI thread waits for it, and for
    // longer than it was busy; only what comes after the wait is the next frame's.
    std::this_thread::sleep_for(60ms);
    window.beforeSynchronizing();
    window.afterSynchronizing();
    std::this_thread::sleep_for(8ms);
    loop->aboutToBlock();
    loop->awake();
    window.afterAnimating();

    const FrameProfiler::Report report = profiler.report();
    REQUIRE(report.animate_frames == 2);
    // The larger of the two is the first, which was busy 15 ms: sleeps last at
    // least what they were asked for, so this cannot fail on a slow machine.
    CHECK(report.animate_max_ms >= 14.0);
    // Both together are the 23 ms the thread was busy, and not the 83 ms it was
    // in the middle of a wakeup for (a slow machine adds to the sleeps, so the
    // bound is loose; the wait alone is 60).
    CHECK(report.animate_mean_ms * 2.0 < 60.0);
}

TEST_CASE("the target is reached once and told on the GUI thread", "[app][frame]") {
    OffscreenApplication app;
    QQuickWindow window;
    FrameProfiler profiler(&window);
    profiler.set_target(2, 1);
    int finished = 0;
    QObject::connect(&profiler, &FrameProfiler::finished, [&finished] { ++finished; });

    // The first swap is the clock starting, the next is the warm-up, then two to
    // measure.
    for (int swap = 0; swap < 6; ++swap) {
        window.frameSwapped();
    }
    CHECK(profiler.recorded_frames() == 2);
    QCoreApplication::processEvents();
    CHECK(finished == 1);
}

TEST_CASE("the profiler can go while the render thread is in the middle of a frame",
          "[app][frame][thread]") {
    OffscreenApplication app;
    QQuickWindow window;

    // The render thread, played by another: it sends the frame's signals as fast as
    // it can, and the profiler is made and destroyed under it. A slot that touched a
    // destroyed profiler is what the sanitizer builds report; the others see a
    // crash or a hang.
    std::atomic<bool> stop{false};
    std::atomic<long> frames{0};
    std::thread render([&] {
        while (!stop.load()) {
            window.beforeFrameBegin();
            window.beforeSynchronizing();
            window.afterSynchronizing();
            window.beforeRenderPassRecording();
            window.afterRenderPassRecording();
            window.afterFrameEnd();
            window.frameSwapped();
            frames.fetch_add(1);
        }
    });

    for (int round = 0; round < 200; ++round) {
        const long before = frames.load();
        {
            FrameProfiler profiler(&window);
            profiler.set_target(5, 0);
            // Let at least a few frames through the slots.
            while (frames.load() < before + 3) {
                std::this_thread::yield();
            }
            // Destroyed here, with the thread still sending.
        }
        QCoreApplication::processEvents();
    }
    stop.store(true);
    render.join();
    CHECK(frames.load() > 600);
}
