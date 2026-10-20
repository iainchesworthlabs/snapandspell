#pragma once

#include <QString>
#include <QStringList>
#include <memory>
#include <optional>
#include <utility>

#include "camera_backend.hpp"
#include "wjs/scan/frame.hpp"

// A camera backend for the tests: no device, no driver and no Qt Multimedia. It
// is handed to CameraSource's second constructor, so the model and the screens
// are driven by a source that really is one; only the frames are made up (a
// synthetic board or a small picture), never a live camera. The test support
// lives here and is not in anything that ships.

namespace wjs::app::test {

class FakeCameraBackend final : public CameraBackend {
public:
    void setChanged(Changed changed) override { changed_ = std::move(changed); }

    [[nodiscard]] bool available() const override { return available_; }

    [[nodiscard]] QStringList devices() const override {
        return available_ ? QStringList{QStringLiteral("Fake camera")} : QStringList{};
    }

    [[nodiscard]] QString problem() const override { return problem_; }

    [[nodiscard]] bool running() const override { return running_; }

    bool start() override {
        if (!available_) {
            problem_ = QStringLiteral("no_camera");
            return false;
        }
        if (!problem_.isEmpty()) {
            // A camera the person refused, or one the platform failed: the source
            // reports the code and stays stopped.
            return false;
        }
        if (!running_) {
            running_ = true;
            notify();
        }
        return true;
    }

    void stop() override {
        if (!running_) {
            return;
        }
        running_ = false;
        notify();
    }

    [[nodiscard]] std::optional<wjs::scan::OwnedFrame> capture() override {
        if (!running_ || !frame_) {
            return std::nullopt;
        }
        std::optional<wjs::scan::OwnedFrame> frame = std::move(frame_);
        frame_.reset();
        return frame;
    }

    void attachPreview(QObject* videoOutput) override {
        if (videoOutput != nullptr) {
            ++previews_;
        }
    }

    void detachPreview() override {}

    // --- what a test says about this camera ---------------------------------
    void setAvailable(bool available) { available_ = available; }
    void setProblem(QString problem) { problem_ = std::move(problem); }
    void setFrame(wjs::scan::OwnedFrame frame) { frame_ = std::move(frame); }
    // A device that fails while running: the source's `changed` follows.
    void fail(QString problem) {
        problem_ = std::move(problem);
        running_ = false;
        notify();
    }
    [[nodiscard]] int previews() const { return previews_; }

private:
    void notify() {
        if (changed_) {
            changed_();
        }
    }

    Changed changed_;
    bool available_ = false;
    bool running_ = false;
    QString problem_ = QStringLiteral("no_camera");
    std::optional<wjs::scan::OwnedFrame> frame_;
    int previews_ = 0;
};

}  // namespace wjs::app::test
