// The Qt Multimedia camera: QCamera into a QMediaCaptureSession, its frames into
// a QVideoSink, and the newest one converted on capture. No ImageCapture, no
// QMediaRecorder and no file: a frame is memory, and the model drops it after the
// read (docs/design/scanner.md, "The Qt Multimedia desktop backend").
//
// This file is compiled only when the kit has Qt Multimedia
// (src/app/CMakeLists.txt). The other variant is
// src/app/variants/camera-none/wjs/app/camera_backend.cpp; both define
// make_camera_backend() and the shared interface is in
// src/app/camera/camera_backend.hpp.

#include "camera_backend.hpp"

#include <QCamera>
#include <QCameraDevice>
#include <QCoreApplication>
#include <QImage>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QPermissions>
#include <QPointer>
#include <QVariant>
#include <QVideoFrame>
#include <QVideoSink>
#include <memory>
#include <optional>
#include <utility>

#include "frame_conversion.hpp"

namespace wjs::app {
namespace {

// The camera the desktop should use first: a rear camera where the device has
// one (a tablet), otherwise the first the platform lists.
[[nodiscard]] QCameraDevice choose_device(const QList<QCameraDevice>& devices) {
    for (const QCameraDevice& device : devices) {
        if (device.position() == QCameraDevice::Position::BackFace) {
            return device;
        }
    }
    return devices.front();
}

}  // namespace

class MultimediaCameraBackend final : public CameraBackend {
public:
    MultimediaCameraBackend() {
        session_.setVideoSink(&sink_);
        QObject::connect(&sink_, &QVideoSink::videoFrameChanged, &owner_,
                         [this](const QVideoFrame& frame) {
                             // A live view may deliver dozens of a second. Only
                             // the newest is kept, and only until it is taken.
                             if (frame.isValid()) {
                                 newest_ = frame;
                             }
                         });
    }

    ~MultimediaCameraBackend() override {
        // Stop first, then take the camera out of the session: the order Qt's
        // own examples use, and the one that leaves no capture running while the
        // objects are torn down.
        if (camera_) {
            camera_->stop();
            session_.setCamera(nullptr);
        }
    }

    void setChanged(Changed changed) override { changed_ = std::move(changed); }

    [[nodiscard]] bool available() const override {
        return !QMediaDevices::videoInputs().isEmpty();
    }

    [[nodiscard]] QStringList devices() const override {
        QStringList names;
        for (const QCameraDevice& device : QMediaDevices::videoInputs()) {
            names.append(device.description().isEmpty() ? QString::fromUtf8(device.id())
                                                        : device.description());
        }
        return names;
    }

    [[nodiscard]] QString problem() const override {
        if (QMediaDevices::videoInputs().isEmpty()) {
            return QStringLiteral("no_camera");
        }
        return problem_;
    }

    [[nodiscard]] bool running() const override {
        return camera_ != nullptr && camera_->isActive();
    }

    bool start() override {
        if (running()) {
            return true;
        }
        const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
        if (cameras.isEmpty()) {
            problem_ = QStringLiteral("no_camera");
            notify();
            return false;
        }
        // The platform asks for the camera permission when the camera starts on
        // the platforms that have one; a refusal already recorded is a fact, not
        // a fault. No permission is requested from here (Qt's camera does that).
        if (QCoreApplication* application = QCoreApplication::instance()) {
            const QCameraPermission permission;
            if (application->checkPermission(permission) == Qt::PermissionStatus::Denied) {
                problem_ = QStringLiteral("permission");
                notify();
                return false;
            }
        }

        const QCameraDevice chosen = choose_device(cameras);
        if (!camera_) {
            camera_ = std::make_unique<QCamera>(chosen);
            session_.setCamera(camera_.get());
            QObject::connect(camera_.get(), &QCamera::errorOccurred, &owner_,
                             [this](QCamera::Error error, const QString&) {
                                 if (error != QCamera::NoError) {
                                     problem_ = QStringLiteral("failed");
                                     if (camera_) {
                                         camera_->stop();
                                     }
                                     notify();
                                 }
                             });
            QObject::connect(camera_.get(), &QCamera::activeChanged, &owner_,
                             [this](bool /*active*/) { notify(); });
        } else if (camera_->cameraDevice() != chosen) {
            camera_->setCameraDevice(chosen);
        }

        problem_.clear();
        // QCamera::start() asks the platform for permission on the platforms
        // that prompt, and sets ActiveState at once. A camera that then fails is
        // an errorOccurred, which sets "failed".
        camera_->start();
        notify();
        return true;
    }

    void stop() override {
        if (camera_) {
            camera_->stop();
        }
        newest_.reset();
        // The sink keeps the last frame it rendered; clearing it here is what
        // makes "the buffer is dropped when the camera stops" true of the
        // preview as well as of the capture.
        sink_.setVideoFrame(QVideoFrame{});
        notify();
    }

    [[nodiscard]] std::optional<wjs::scan::OwnedFrame> capture() override {
        if (!newest_ || !newest_->isValid()) {
            return std::nullopt;
        }
        // The frame is taken out of the mailbox and converted; what the sink
        // holds is a shared buffer, and nothing keeps it after this call.
        const QVideoFrame frame = std::move(*newest_);
        newest_.reset();
        const QImage image = frame.toImage();
        if (image.isNull()) {
            problem_ = QStringLiteral("failed");
            notify();
            return std::nullopt;
        }
        std::optional<wjs::scan::OwnedFrame> owned = frame_from_image(image);
        if (!owned) {
            problem_ = QStringLiteral("failed");
            notify();
        }
        return owned;
    }

    void attachPreview(QObject* videoOutput) override {
        preview_ = videoOutput;
        if (preview_ != nullptr) {
            // VideoOutput's "videoSink" property. Set by name so this header,
            // and every file that includes it, names no Qt Multimedia type.
            preview_->setProperty("videoSink", QVariant::fromValue(&sink_));
        }
    }

    void detachPreview() override {
        if (preview_ != nullptr) {
            preview_->setProperty("videoSink",
                                  QVariant::fromValue(static_cast<QVideoSink*>(nullptr)));
        }
        preview_.clear();
    }

private:
    void notify() {
        if (changed_) {
            changed_();
        }
    }

    // A QObject for the connections' receiver context: it keeps them alive while
    // this backend lives, and drops them before the sink and the camera go.
    QObject owner_;
    QMediaCaptureSession session_;
    QVideoSink sink_;
    std::unique_ptr<QCamera> camera_;
    QPointer<QObject> preview_;
    std::optional<QVideoFrame> newest_;
    QString problem_;
    Changed changed_;
};

std::unique_ptr<CameraBackend> make_camera_backend() {
    return std::make_unique<MultimediaCameraBackend>();
}

}  // namespace wjs::app
