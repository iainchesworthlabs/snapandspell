#include "scan_model.hpp"

#include <QVariantMap>
#include <cstddef>
#include <string>
#include <utility>

#include "wjs/platform/board_reader.hpp"
#include "wjs/platform/camera.hpp"
#include "wjs/profiling.hpp"
#include "wjs/scan/reader.hpp"

namespace wjs::app {
namespace {

// How often a capture looks for a frame, and for how long: a camera that has
// just started has not delivered one yet, and a platform whose camera never
// does must not leave the screen in `reading`. The numbers are the screen's
// patience, not a rule of the scanner.
constexpr int kFramePollMs = 100;
constexpr int kMaxFramePolls = 40;

[[nodiscard]] QString camera_code(wjs::platform::camera::CameraError error) {
    switch (error) {
        case wjs::platform::camera::CameraError::unavailable:
            return QStringLiteral("unavailable");
        case wjs::platform::camera::CameraError::permission_denied:
            return QStringLiteral("permission_denied");
        case wjs::platform::camera::CameraError::no_such_device:
            return QStringLiteral("no_such_device");
        case wjs::platform::camera::CameraError::already_running:
            return QStringLiteral("already_running");
        case wjs::platform::camera::CameraError::failed:
            return QStringLiteral("failed");
    }
    return QStringLiteral("failed");
}

[[nodiscard]] QString read_code(wjs::scan::ReadError error) {
    switch (error) {
        case wjs::scan::ReadError::invalid_frame:
            return QStringLiteral("invalid_frame");
        case wjs::scan::ReadError::board_not_found:
            return QStringLiteral("board_not_found");
        case wjs::scan::ReadError::size_not_found:
            return QStringLiteral("size_not_found");
        case wjs::scan::ReadError::no_letters:
            return QStringLiteral("no_letters");
        case wjs::scan::ReadError::invalid_options:
            return QStringLiteral("invalid_options");
    }
    return QStringLiteral("board_not_found");
}

[[nodiscard]] QString origin_name(wjs::scan::TileOrigin origin) {
    switch (origin) {
        case wjs::scan::TileOrigin::read:
            return QStringLiteral("read");
        case wjs::scan::TileOrigin::corrected:
            return QStringLiteral("corrected");
        case wjs::scan::TileOrigin::typed:
            return QStringLiteral("typed");
        case wjs::scan::TileOrigin::empty:
            return QStringLiteral("empty");
    }
    return QStringLiteral("empty");
}

[[nodiscard]] QString to_qstring(const std::string& text) {
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

}  // namespace

ScanModel::ScanModel(QObject* parent) : QObject(parent) {
    frame_timer_.setInterval(kFramePollMs);
    frame_timer_.setSingleShot(false);
    connect(&frame_timer_, &QTimer::timeout, this, &ScanModel::tryFrame);
}

ScanModel::~ScanModel() {
    close();
}

QString ScanModel::state() const {
    switch (state_) {
        case State::ready:
            return QStringLiteral("ready");
        case State::reading:
            return QStringLiteral("reading");
        case State::check:
            return QStringLiteral("check");
    }
    return QStringLiteral("ready");
}

void ScanModel::setState(State state) {
    if (state_ != state) {
        state_ = state;
        emit stateChanged();
    }
}

int ScanModel::rows() const {
    return draft_ ? static_cast<int>(draft_->rows()) : 0;
}

int ScanModel::columns() const {
    return draft_ ? static_cast<int>(draft_->columns()) : 0;
}

int ScanModel::size() const {
    return draft_ ? static_cast<int>(draft_->rows()) : 0;
}

QVariantList ScanModel::sizes() const {
    QVariantList list;
    for (const int size : {3, 4, 5, 6}) {
        list.append(size);
    }
    return list;
}

QStringList ScanModel::letters() const {
    QStringList list;
    if (!draft_) {
        return list;
    }
    for (std::size_t row = 0; row < draft_->rows(); ++row) {
        for (std::size_t column = 0; column < draft_->columns(); ++column) {
            list.append(to_qstring(draft_->tile({row, column}).letters));
        }
    }
    return list;
}

QVariantList ScanModel::tiles() const {
    QVariantList list;
    if (!draft_) {
        return list;
    }
    for (std::size_t row = 0; row < draft_->rows(); ++row) {
        for (std::size_t column = 0; column < draft_->columns(); ++column) {
            const wjs::scan::DraftTile& tile = draft_->tile({row, column});
            QVariantMap map;
            map.insert(QStringLiteral("letters"), to_qstring(tile.letters));
            map.insert(QStringLiteral("doubtful"), tile.doubtful);
            map.insert(QStringLiteral("origin"), origin_name(tile.origin));
            QStringList alternatives;
            for (const wjs::scan::Alternative& alternative : tile.alternatives) {
                if (!alternative.letters.empty()) {
                    alternatives.append(to_qstring(alternative.letters));
                }
            }
            map.insert(QStringLiteral("alternatives"), alternatives);
            list.append(map);
        }
    }
    return list;
}

QVariantList ScanModel::doubtful() const {
    QVariantList list;
    if (!draft_) {
        return list;
    }
    for (const wjs::Position& position : draft_->doubtful()) {
        list.append(static_cast<int>(position.row * draft_->columns() + position.column));
    }
    return list;
}

bool ScanModel::complete() const {
    return draft_ && draft_->complete();
}

bool ScanModel::on_grid(int index) const {
    if (!draft_ || index < 0) {
        return false;
    }
    return static_cast<std::size_t>(index) < draft_->rows() * draft_->columns();
}

QStringList ScanModel::alternatives(int index) const {
    QStringList list;
    if (!draft_ || !on_grid(index)) {
        return list;
    }
    const auto i = static_cast<std::size_t>(index);
    const wjs::scan::DraftTile& tile = draft_->tile({i / draft_->columns(), i % draft_->columns()});
    for (const wjs::scan::Alternative& alternative : tile.alternatives) {
        if (!alternative.letters.empty()) {
            list.append(to_qstring(alternative.letters));
        }
    }
    return list;
}

// --- the camera ---------------------------------------------------------------

void ScanModel::setCameraSource(CameraSource* source) {
    if (camera_source_ == source) {
        return;
    }
    // A source being replaced is not left running: the camera is only ever on
    // for the screen that is waiting for a frame.
    if (camera_capturing_) {
        stopCamera();
    }
    if (camera_source_) {
        disconnect(camera_source_.data(), nullptr, this, nullptr);
    }
    camera_source_ = source;
    if (camera_source_) {
        connect(camera_source_.data(), &CameraSource::changed, this,
                &ScanModel::refreshCameraState);
        // The source is not owned here. If it goes first, the model says it has
        // no camera from then on, rather than holding one QML could still ask
        // for.
        connect(camera_source_.data(), &QObject::destroyed, this, [this] {
            camera_source_ = nullptr;
            camera_available_ = false;
            camera_capturing_ = false;
            emit cameraChanged();
            emit cameraSourceChanged();
        });
    }
    camera_available_ = false;
    camera_capturing_ = false;
    camera_problem_.clear();
    emit cameraChanged();
    emit cameraSourceChanged();
    // A source that is already running (a test handing in a fake) is described
    // at once; a real one is started by open(), on scan.ready.
    refreshCameraState();
}

void ScanModel::refreshCameraState() {
    if (!camera_source_) {
        return;
    }
    const bool running = camera_source_->running();
    const QString problem = camera_source_->problem();
    bool changed = false;
    if (camera_capturing_ != running) {
        camera_capturing_ = running;
        changed = true;
    }
    if (running) {
        if (!camera_available_ || !camera_problem_.isEmpty()) {
            camera_available_ = true;
            camera_problem_.clear();
            changed = true;
        }
    } else if (!problem.isEmpty()) {
        if (camera_available_ || camera_problem_ != problem) {
            camera_available_ = false;
            camera_problem_ = problem;
            changed = true;
        }
    }
    if (changed) {
        emit cameraChanged();
    }
}

void ScanModel::open() {
    // The camera is on only in `ready`: once a frame is taken or the check screen
    // is open, nothing needs it (docs/design/game.md, "The MVP: one capture").
    if (state_ != State::ready || camera_capturing_) {
        return;
    }
    camera_available_ = false;
    camera_problem_.clear();
    if (camera_source_) {
        // The application supplied a CameraSource: the live view and the frame
        // both come from it. It answers for a device with no camera, a refusal
        // and a failure with a code; nothing is faked and the typed board stays
        // beside it (docs/design/game.md, "No camera").
        camera_source_->start();
        camera_available_ = camera_source_->running();
        camera_capturing_ = camera_available_;
        if (!camera_available_) {
            camera_problem_ = camera_source_->problem();
            if (camera_problem_.isEmpty()) {
                camera_problem_ = QStringLiteral("no_camera");
            }
        }
        emit cameraChanged();
        return;
    }
    const auto devices = wjs::platform::camera::enumerate_devices();
    if (!devices) {
        camera_problem_ = camera_code(devices.error());
        emit cameraChanged();
        return;
    }
    if (devices->empty()) {
        // Every desktop without a CameraSource: the platform interface has no
        // backend, and the screen offers the typed board.
        camera_problem_ = QStringLiteral("unavailable");
        emit cameraChanged();
        return;
    }
    const wjs::platform::camera::DeviceInfo* chosen = &devices->front();
    for (const wjs::platform::camera::DeviceInfo& device : *devices) {
        if (device.facing == wjs::platform::camera::Facing::rear) {
            chosen = &device;
            break;
        }
    }
    const auto started = wjs::platform::camera::start(*chosen);
    if (!started) {
        camera_problem_ = camera_code(started.error());
        emit cameraChanged();
        return;
    }
    camera_available_ = true;
    camera_capturing_ = true;
    emit cameraChanged();
}

void ScanModel::stopCamera() {
    frame_timer_.stop();
    if (camera_source_) {
        camera_source_->stop();
    } else {
        wjs::platform::camera::stop();
    }
    if (camera_capturing_) {
        camera_capturing_ = false;
        emit cameraChanged();
    }
}

void ScanModel::close() {
    stopCamera();
}

void ScanModel::capture() {
    if (state_ != State::ready) {
        return;
    }
    if (!read_problem_.isEmpty()) {
        read_problem_.clear();
        emit readProblemChanged();
    }
    if (!camera_capturing_) {
        // No camera, or one this platform will not start: the typed board is the
        // way on, and the screen says so (cameraProblem).
        read_problem_ = QStringLiteral("no_camera");
        emit readProblemChanged();
        return;
    }
    frame_ticks_ = 0;
    setState(State::reading);
    frame_timer_.start();
    tryFrame();
}

void ScanModel::tryFrame() {
    if (camera_source_) {
        if (!camera_source_->capture()) {
            if (++frame_ticks_ > kMaxFramePolls) {
                failed(QStringLiteral("no_frame"));
            }
            return;
        }
        std::optional<wjs::scan::OwnedFrame> frame = camera_source_->takeFrame();
        if (!frame) {
            // The source said it had one and did not: a camera that cannot hand
            // its frame over is a failed read, not a screen left in `reading`.
            failed(QStringLiteral("no_frame"));
            return;
        }
        readFrame(*frame);
        return;
    }

    const std::optional<wjs::scan::OwnedFrame> frame =
        wjs::platform::camera::try_pop_latest_frame();
    if (!frame) {
        if (++frame_ticks_ > kMaxFramePolls) {
            failed(QStringLiteral("no_frame"));
        }
        return;
    }
    readFrame(*frame);
}

void ScanModel::readFrame(const wjs::scan::OwnedFrame& frame) {
    // The model's whole per-frame work: one captured frame becomes a reading or
    // a reason why not (the camera poll that fetched it is not in here).
    auto zone = wjs::profiling::zone<"scan.process_frame">();
    frame_timer_.stop();
    // The camera stops as soon as a frame is in hand, before the read: the
    // picture of a room is in memory, and the next one is not needed.
    stopCamera();
    const std::expected<wjs::scan::BoardReading, wjs::scan::ReadError> reading =
        wjs::platform::board_reader::read(frame.view());
    // The frame is a picture of a room and a child may be in it. It is read here
    // and gone: only the reading's letters and doubts go on to the check screen.
    if (!reading) {
        failed(read_code(reading.error()));
        return;
    }
    if (!useReading(*reading)) {
        failed(QStringLiteral("invalid_reading"));
    }
}

void ScanModel::failed(const QString& code) {
    stopCamera();
    read_problem_ = code;
    setState(State::ready);
    emit readProblemChanged();
    // Back on the ready screen: the camera is on again for another go.
    open();
}

void ScanModel::typeBoard() {
    close();
    const auto empty = wjs::scan::BoardDraft::empty(4, 4);
    if (!empty) {
        return;
    }
    draft_ = *empty;
    draft_from_camera_ = false;
    confirmed_.reset();
    if (!read_problem_.isEmpty()) {
        read_problem_.clear();
        emit readProblemChanged();
    }
    emit draftChanged();
    emit confirmedChanged();
    setState(State::check);
}

void ScanModel::readAgain() {
    close();
    draft_.reset();
    draft_from_camera_ = false;
    confirmed_.reset();
    if (!read_problem_.isEmpty()) {
        read_problem_.clear();
        emit readProblemChanged();
    }
    emit draftChanged();
    emit confirmedChanged();
    setState(State::ready);
    open();
}

bool ScanModel::useReading(const wjs::scan::BoardReading& reading) {
    if (reading.rows == 0 || reading.columns == 0 || reading.rows > wjs::Board::max_dimension ||
        reading.columns > wjs::Board::max_dimension ||
        reading.tiles.size() != reading.rows * reading.columns) {
        return false;
    }
    draft_ = wjs::scan::BoardDraft::from_reading(reading);
    draft_from_camera_ = true;
    confirmed_.reset();
    if (!read_problem_.isEmpty()) {
        read_problem_.clear();
        emit readProblemChanged();
    }
    emit draftChanged();
    emit confirmedChanged();
    setState(State::check);
    return true;
}

// --- the check screen ----------------------------------------------------------

void ScanModel::changedDraft() {
    confirmed_.reset();
    emit confirmedChanged();
    emit draftChanged();
}

bool ScanModel::setTile(int index, const QString& letters) {
    if (!draft_ || !on_grid(index)) {
        return false;
    }
    const auto i = static_cast<std::size_t>(index);
    const std::string text = letters.toStdString();
    const auto set = draft_->set_tile({i / draft_->columns(), i % draft_->columns()}, text);
    if (!set) {
        return false;
    }
    changedDraft();
    return true;
}

bool ScanModel::acceptTile(int index) {
    if (!draft_ || !on_grid(index)) {
        return false;
    }
    const auto i = static_cast<std::size_t>(index);
    const auto accepted = draft_->accept_tile({i / draft_->columns(), i % draft_->columns()});
    if (!accepted) {
        return false;
    }
    changedDraft();
    return true;
}

bool ScanModel::clearTile(int index) {
    if (!draft_ || !on_grid(index)) {
        return false;
    }
    const auto i = static_cast<std::size_t>(index);
    const auto cleared = draft_->clear_tile({i / draft_->columns(), i % draft_->columns()});
    if (!cleared) {
        return false;
    }
    changedDraft();
    return true;
}

void ScanModel::turn() {
    if (!draft_) {
        return;
    }
    draft_->turn(wjs::scan::Turn::clockwise);
    changedDraft();
}

bool ScanModel::resize(int rows, int columns) {
    if (!draft_ || rows <= 0 || columns <= 0) {
        return false;
    }
    const auto resized =
        draft_->resize(static_cast<std::size_t>(rows), static_cast<std::size_t>(columns));
    if (!resized) {
        return false;
    }
    changedDraft();
    return true;
}

bool ScanModel::confirm() {
    if (!draft_) {
        return false;
    }
    auto board = draft_->confirm();
    if (!board) {
        return false;
    }
    confirmed_ = std::move(*board);
    emit confirmedChanged();
    return true;
}

void ScanModel::retryCamera() {
    if (state_ != State::ready) {
        return;
    }
    // A retry is a new attempt: the failed read's sentence is not about this
    // one. The camera's own answer replaces the problem when it fails again.
    if (!read_problem_.isEmpty()) {
        read_problem_.clear();
        emit readProblemChanged();
    }
    // open() enumerates the devices again (or starts the supplied source again)
    // and says what the camera did; nothing is faked and nothing is kept.
    open();
}

}  // namespace wjs::app
