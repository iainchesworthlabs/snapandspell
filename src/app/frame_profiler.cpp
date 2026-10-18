#include "frame_profiler.hpp"

#include <QElapsedTimer>
#include <QMutexLocker>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "wjs/profiling.hpp"

namespace wjs::app {
namespace {

QElapsedTimer& process_clock() {
    static QElapsedTimer clock;
    return clock;
}

// The value a fraction of the way through a sorted list, interpolated between
// its neighbours as FrameStats does for the prototype's tables.
double percentile(std::vector<double> values, double fraction) {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const double position = fraction * static_cast<double>(values.size() - 1);
    const auto below = static_cast<std::size_t>(std::floor(position));
    const auto above = static_cast<std::size_t>(std::ceil(position));
    const double weight = position - static_cast<double>(below);
    return values[below] * (1.0 - weight) + values[above] * weight;
}

double mean(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (const double value : values) {
        sum += value;
    }
    return sum / static_cast<double>(values.size());
}

double largest(const std::vector<double>& values) {
    return values.empty() ? 0.0 : *std::max_element(values.begin(), values.end());
}

int count_above(const std::vector<double>& values, double limit) {
    return static_cast<int>(std::count_if(values.begin(), values.end(),
                                          [limit](double value) { return value > limit; }));
}

QString two(double value) {
    return QString::number(value, 'f', 2);
}

}  // namespace

void FrameProfiler::Window::push(double value, double at) noexcept {
    values[static_cast<std::size_t>(next)] = value;
    next = (next + 1) % kWindow;
    if (count < kWindow) {
        ++count;
    }
    if (count == 1) {
        first_at = at;
    }
    last_at = at;
}

void FrameProfiler::Window::clear() noexcept {
    count = 0;
    next = 0;
    first_at = 0.0;
    last_at = 0.0;
}

std::vector<double> FrameProfiler::Window::in_order() const {
    std::vector<double> out;
    out.reserve(static_cast<std::size_t>(count));
    // The oldest value is the next to be overwritten once the window is full.
    const int start = count < kWindow ? 0 : next;
    for (int i = 0; i < count; ++i) {
        out.push_back(values[static_cast<std::size_t>((start + i) % kWindow)]);
    }
    return out;
}

FrameProfiler::FrameProfiler(QQuickWindow* window, QObject* parent) : QObject(parent) {
    if (!process_clock().isValid()) {
        process_clock().start();
    }
    set_window(window);
}

FrameProfiler::~FrameProfiler() {
    if (window_ != nullptr) {
        disconnect(window_, nullptr, this, nullptr);
    }
}

void FrameProfiler::start_process_clock() {
    process_clock().start();
}

double FrameProfiler::now_ms() {
    return static_cast<double>(process_clock().nsecsElapsed()) / 1.0e6;
}

void FrameProfiler::set_window(QQuickWindow* window) {
    if (window == window_) {
        return;
    }
    if (window_ != nullptr) {
        disconnect(window_, nullptr, this, nullptr);
    }
    window_ = window;
    if (window_ == nullptr) {
        return;
    }
    // Every one of these is emitted by the render thread. The slots stamp a
    // time and send a zone, plot or mark; they do not touch the window.
    connect(window_, &QQuickWindow::frameSwapped, this, &FrameProfiler::on_swapped,
            Qt::DirectConnection);
    connect(window_, &QQuickWindow::beforeFrameBegin, this, &FrameProfiler::on_frame_begin,
            Qt::DirectConnection);
    connect(window_, &QQuickWindow::afterFrameEnd, this, &FrameProfiler::on_frame_end,
            Qt::DirectConnection);
    connect(window_, &QQuickWindow::beforeSynchronizing, this, &FrameProfiler::on_sync_begin,
            Qt::DirectConnection);
    connect(window_, &QQuickWindow::afterSynchronizing, this, &FrameProfiler::on_sync_end,
            Qt::DirectConnection);
    connect(window_, &QQuickWindow::sceneGraphInitialized, this,
            &FrameProfiler::on_scene_graph_initialized, Qt::DirectConnection);
}

void FrameProfiler::set_target(int frames, int warmup) {
    const QMutexLocker locker(&mutex_);
    target_ = std::max(0, frames);
    warmup_ = std::max(0, warmup);
    warmup_remaining_ = warmup_;
    measured_ = 0;
    finished_posted_ = false;
    intervals_.clear();
    render_.clear();
    sync_.clear();
}

int FrameProfiler::recorded_frames() const {
    const QMutexLocker locker(&mutex_);
    return intervals_.count;
}

bool FrameProfiler::first_frame_seen() const {
    const QMutexLocker locker(&mutex_);
    return have_previous_;
}

void FrameProfiler::on_scene_graph_initialized() {
    // Once per thread: every call adds a table entry. The scene graph can be
    // initialised again if the window is moved to another renderer; the name
    // is already right.
    if (!render_thread_named_.exchange(true)) {
        wjs::profiling::thread_name("render");
    }
}

void FrameProfiler::on_frame_begin() {
    frame_begin_ = now_ms();
}

void FrameProfiler::on_frame_end() {
    const double at = now_ms();
    const double elapsed = at - frame_begin_;
    {
        const QMutexLocker locker(&mutex_);
        render_.push(elapsed, at);
    }
    wjs::profiling::plot("render.frame_ms", elapsed);
}

void FrameProfiler::on_sync_begin() {
    sync_begin_ = now_ms();
}

void FrameProfiler::on_sync_end() {
    const double at = now_ms();
    const double elapsed = at - sync_begin_;
    {
        const QMutexLocker locker(&mutex_);
        sync_.push(elapsed, at);
    }
    wjs::profiling::plot("ui.sync_ms", elapsed);
}

void FrameProfiler::on_swapped() {
    const double at = now_ms();
    bool first = false;
    bool reached = false;
    double interval = 0.0;
    {
        const QMutexLocker locker(&mutex_);
        if (!have_previous_) {
            have_previous_ = true;
            first = true;
            first_frame_ms_ = at;
        } else {
            interval = at - previous_;
            if (warmup_remaining_ > 0) {
                --warmup_remaining_;
            } else if (target_ == 0 || measured_ < target_) {
                intervals_.push(interval, at);
                ++measured_;
                if (target_ > 0 && measured_ >= target_ && !finished_posted_) {
                    finished_posted_ = true;
                    reached = true;
                }
            }
        }
        previous_ = at;
    }

    wjs::profiling::frame_mark("UI");
    if (first) {
        // Launch to first frame: the one number a frame interval cannot hold,
        // because it is the wait before the first of them.
        wjs::profiling::plot("ui.first_frame_ms", at);
    } else {
        wjs::profiling::plot("ui.frame_ms", interval);
    }

    if (reached) {
        QMetaObject::invokeMethod(this, &FrameProfiler::finish_guarded, Qt::QueuedConnection);
    }
}

void FrameProfiler::finish_guarded() {
    emit finished();
}

FrameProfiler::Report FrameProfiler::report() const {
    std::vector<double> intervals;
    std::vector<double> render;
    std::vector<double> sync;
    double span = 0.0;
    {
        const QMutexLocker locker(&mutex_);
        intervals = intervals_.in_order();
        render = render_.in_order();
        sync = sync_.in_order();
        span = intervals_.count > 0 ? intervals_.last_at - intervals_.first_at : 0.0;
        Report out;
        out.frames = static_cast<int>(intervals.size());
        out.warmup = warmup_;
        out.span_ms = span;
        out.mean_ms = mean(intervals);
        out.p50_ms = percentile(intervals, 0.50);
        out.p95_ms = percentile(intervals, 0.95);
        out.p99_ms = percentile(intervals, 0.99);
        out.max_ms = largest(intervals);
        out.over_16_7 = count_above(intervals, 16.7);
        out.over_33 = count_above(intervals, 33.0);
        out.first_frame_ms = first_frame_ms_;

        out.render_frames = static_cast<int>(render.size());
        out.render_mean_ms = mean(render);
        out.render_p95_ms = percentile(render, 0.95);
        out.render_max_ms = largest(render);

        out.sync_frames = static_cast<int>(sync.size());
        out.sync_mean_ms = mean(sync);
        out.sync_p95_ms = percentile(sync, 0.95);
        out.sync_max_ms = largest(sync);
        return out;
    }
}

QString FrameProfiler::report_text() const {
    const Report r = report();
    const double over_share =
        r.frames > 0 ? 100.0 * static_cast<double>(r.over_16_7) / static_cast<double>(r.frames)
                     : 0.0;

    QString text;
    // The one line a machine reads (tests/app/frame_timing_test.cpp and the
    // capture recipes): key=value, no spaces in a value.
    text += QStringLiteral(
                "smoke: frames=%1 warmup=%2 span_ms=%3 p50_ms=%4 p95_ms=%5 p99_ms=%6 "
                "max_ms=%7 over16_7=%8 over33=%9 first_frame_ms=%10 render_mean_ms=%11 "
                "render_max_ms=%12 sync_mean_ms=%13 sync_max_ms=%14\n")
                .arg(r.frames)
                .arg(r.warmup)
                .arg(two(r.span_ms))
                .arg(two(r.p50_ms))
                .arg(two(r.p95_ms))
                .arg(two(r.p99_ms))
                .arg(two(r.max_ms))
                .arg(r.over_16_7)
                .arg(r.over_33)
                .arg(two(r.first_frame_ms))
                .arg(two(r.render_mean_ms))
                .arg(two(r.render_max_ms))
                .arg(two(r.sync_mean_ms))
                .arg(two(r.sync_max_ms));

    text += QStringLiteral(
                "ui.frame_ms: %1 frames over %2 ms: mean %3, p50 %4, p95 %5, p99 %6, max %7 "
                "ms\n")
                .arg(r.frames)
                .arg(two(r.span_ms))
                .arg(two(r.mean_ms))
                .arg(two(r.p50_ms))
                .arg(two(r.p95_ms))
                .arg(two(r.p99_ms))
                .arg(two(r.max_ms));
    text += QStringLiteral("ui.frame_ms over 16.7 ms: %1 (%2%)   over 33 ms: %3\n")
                .arg(r.over_16_7)
                .arg(two(over_share))
                .arg(r.over_33);
    text += QStringLiteral("ui.first_frame_ms: %1 ms (process start to the first frame)\n")
                .arg(two(r.first_frame_ms));
    text += QStringLiteral(
                "render.frame_ms: %1 frames, mean %2, p95 %3, max %4 ms (the render thread's "
                "own time for a frame)\n")
                .arg(r.render_frames)
                .arg(two(r.render_mean_ms))
                .arg(two(r.render_p95_ms))
                .arg(two(r.render_max_ms));
    text += QStringLiteral(
                "ui.sync_ms: %1 frames, mean %2, p95 %3, max %4 ms (the render thread waiting "
                "for the GUI thread)\n")
                .arg(r.sync_frames)
                .arg(two(r.sync_mean_ms))
                .arg(two(r.sync_p95_ms))
                .arg(two(r.sync_max_ms));
    return text;
}

}  // namespace wjs::app
