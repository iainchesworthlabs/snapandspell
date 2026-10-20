#pragma once

#include <QObject>
#include <QQuickWindow>
#include <QString>

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

#include <QMutex>

namespace wjs::app {

// The frame clock of the window (docs/profiling.md, "The frame"): a frame mark
// named "UI" at every swap, the interval between swaps as the plot
// `ui.frame_ms`, the render thread's own time and its wait for the GUI thread,
// and a rolling window of all of it from which a report is made.
//
// The swap is where a frame ends, so frameSwapped is connected directly: the
// slot stamps a monotonic time and sends the mark and the plot where the frame
// ended (the render thread of Qt's threaded loop, the GUI thread of the basic
// one). It never blocks the render thread beyond a short mutex around one
// number, and it never touches the window. The GUI thread names itself "main";
// the render thread is named "render" the first time the scene graph is
// initialised.
//
// The report is what the smoke mode prints: count, mean, p50, p95, p99 and max
// of the frame interval, the frames over 16.7 and over 33 ms, and the first
// frame's time from the start of the process. It is measured on delivery of
// frameSwapped, so a frame the compositor only presented late counts late.
class FrameProfiler : public QObject {
    Q_OBJECT

public:
    // Attaches to `window`, which must outlive this. The window is the one the
    // QML engine made; the application sets it up after loadFromModule and
    // before exec, so the first frame is measured.
    explicit FrameProfiler(QQuickWindow* window, QObject* parent = nullptr);
    ~FrameProfiler() override;

    FrameProfiler(const FrameProfiler&) = delete;
    FrameProfiler& operator=(const FrameProfiler&) = delete;

    // Starts the process clock. Called first thing in main, so that "time from
    // the start of the process" includes Qt's own start-up.
    static void start_process_clock();
    // Milliseconds since start_process_clock().
    [[nodiscard]] static double now_ms();

    // Records `frames` frame intervals after leaving out the first `warmup` of
    // them (the window's first frames create the scene and its textures, and
    // are not a steady frame), then emits finished() on the GUI thread. A
    // target of 0 -- the default -- only keeps the rolling window.
    void set_target(int frames, int warmup);

    void set_window(QQuickWindow* window);

    [[nodiscard]] int recorded_frames() const;
    [[nodiscard]] bool first_frame_seen() const;

    struct Report {
        int frames = 0;
        int warmup = 0;
        double span_ms = 0.0;
        double mean_ms = 0.0;
        double p50_ms = 0.0;
        double p95_ms = 0.0;
        double p99_ms = 0.0;
        double max_ms = 0.0;
        int over_16_7 = 0;
        int over_33 = 0;

        double first_frame_ms = 0.0;

        int render_frames = 0;
        double render_mean_ms = 0.0;
        double render_p95_ms = 0.0;
        double render_max_ms = 0.0;

        int sync_frames = 0;
        double sync_mean_ms = 0.0;
        double sync_p95_ms = 0.0;
        double sync_max_ms = 0.0;
    };

    [[nodiscard]] Report report() const;
    // The report as the smoke mode prints it: one machine-readable summary line
    // (`smoke: frames=... p50_ms=...`) and one line for each thing it measured.
    [[nodiscard]] QString report_text() const;

signals:
    // The target set with set_target() was reached. Emitted on the GUI thread.
    void finished();

private:
    // A rolling window: the last kWindow values, the oldest overwritten. A
    // frame's value is one double, so a program left running for an hour keeps
    // the last minute or so of frames rather than growing.
    static constexpr int kWindow = 4096;

    struct Window {
        std::array<double, kWindow> values{};
        int count = 0;  // how many of the slots hold a value
        int next = 0;   // where the next value goes
        double first_at = 0.0;
        double last_at = 0.0;

        void push(double value, double at) noexcept;
        void clear() noexcept;
        [[nodiscard]] std::vector<double> in_order() const;
    };

    void on_swapped();
    void on_scene_graph_initialized();
    void on_frame_begin();
    void on_frame_end();
    void on_sync_begin();
    void on_sync_end();
    void finish_guarded();

    QQuickWindow* window_ = nullptr;
    mutable QMutex mutex_;
    Window intervals_;
    Window render_;
    Window sync_;
    bool have_previous_ = false;
    double previous_ = 0.0;
    double first_frame_ms_ = 0.0;
    int measured_ = 0;
    int warmup_remaining_ = 0;
    int target_ = 0;
    int warmup_ = 0;
    bool finished_posted_ = false;

    std::atomic<bool> render_thread_named_{false};

    // Written and read by the render thread only.
    double frame_begin_ = 0.0;
    double sync_begin_ = 0.0;
};

}  // namespace wjs::app
