#pragma once

#include <QtQml/qqmlregistration.h>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <cstddef>
#include <optional>
#include <string>

#include "wjs/board.hpp"
#include "wjs/scan/confirmation.hpp"
#include "wjs/scan/reading.hpp"

namespace wjs::app {

// The check screen's model (`scan.ready`, `scan.reading` and `scan.check`): the
// wjs::scan::BoardDraft a grown-up confirms, the camera's little lifecycle, and
// nothing else. It is the on-device path of docs/design/game.md, "Mode A":
//
//   ready ──capture()──▶ reading ──the reader's reading──▶ check
//     ▲                     │                                  │
//     └──── readAgain() ◀───┴──── a frame that could not be read┘
//
// The camera is on only while the screen is in `ready` and a capture is being
// waited for: open() when the ready screen is shown, close() when it goes (and
// on the frame, at once). Frames are read through the platform's board_reader
// (src/platforms/interfaces/wjs/platform/board_reader.hpp) and are never kept:
// the frame is popped, read and forgotten, and only the BoardReading's letters
// and doubts remain. On every desktop the camera interface answers
// `unavailable`, so the screen offers "Type in your board" with an empty grid
// (docs/design/game.md, "No camera"); the reading path is the same one a
// platform with a camera uses.
//
// A reading never becomes a Board here: confirm() calls BoardDraft::confirm(),
// which is the grown-up's "Yes, play", and the only way a board reaches the
// solver (docs/design/scanner.md, "Confirm and correct").
class ScanModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    // "ready", "reading" or "check".
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)

    // The draft: the grid a grown-up is looking at. Empty before typeBoard() or a
    // reading. `tiles` is one map a tile, row-major: letters, doubtful, origin
    // ("read", "corrected", "typed" or "empty") and alternatives (the reader's
    // other thoughts, for the picker to offer first).
    Q_PROPERTY(int rows READ rows NOTIFY draftChanged)
    Q_PROPERTY(int columns READ columns NOTIFY draftChanged)
    Q_PROPERTY(QStringList letters READ letters NOTIFY draftChanged)
    Q_PROPERTY(QVariantList tiles READ tiles NOTIFY draftChanged)
    // The indices to ring: the tiles the reader doubted and no one has settled.
    Q_PROPERTY(QVariantList doubtful READ doubtful NOTIFY draftChanged)
    Q_PROPERTY(bool complete READ complete NOTIFY draftChanged)

    // The board size, and those the check screen offers. The size is counted
    // from the picture first; the adult can change it (docs/design/game.md,
    // "The check screen"); the MVP's board is a 4x4.
    Q_PROPERTY(int size READ size NOTIFY draftChanged)
    Q_PROPERTY(QVariantList sizes READ sizes CONSTANT)

    // The camera. False on every desktop today, with the reason as a code the
    // wording file turns into a sentence: "unavailable", "permission_denied",
    // "no_such_device", "already_running" or "failed". `cameraProblem` is empty
    // when there is none. `readProblem` is the last failed read's code, cleared
    // by the next capture or readAgain(): "no_camera", "no_frame",
    // "invalid_frame", "board_not_found", "size_not_found", "no_letters",
    // "invalid_reading".
    Q_PROPERTY(bool cameraAvailable READ cameraAvailable NOTIFY cameraChanged)
    Q_PROPERTY(bool cameraCapturing READ cameraCapturing NOTIFY cameraChanged)
    Q_PROPERTY(QString cameraProblem READ cameraProblem NOTIFY cameraChanged)
    Q_PROPERTY(QString readProblem READ readProblem NOTIFY readProblemChanged)

public:
    explicit ScanModel(QObject* parent = nullptr);
    ~ScanModel() override;

    [[nodiscard]] QString state() const;
    [[nodiscard]] int rows() const;
    [[nodiscard]] int columns() const;
    [[nodiscard]] QStringList letters() const;
    [[nodiscard]] QVariantList tiles() const;
    [[nodiscard]] QVariantList doubtful() const;
    [[nodiscard]] bool complete() const;
    [[nodiscard]] int size() const;
    [[nodiscard]] QVariantList sizes() const;
    [[nodiscard]] bool cameraAvailable() const noexcept { return camera_available_; }
    [[nodiscard]] bool cameraCapturing() const noexcept { return camera_capturing_; }
    [[nodiscard]] QString cameraProblem() const { return camera_problem_; }
    [[nodiscard]] QString readProblem() const { return read_problem_; }

    // --- the camera, while the ready screen is shown ------------------------
    // Enumerates the devices and starts the first rear one (the first of any
    // otherwise). Does nothing outside `ready`.
    Q_INVOKABLE void open();
    // Stops the camera and the frame wait. Safe more than once, and when
    // nothing was running.
    Q_INVOKABLE void close();

    // "Capture": waits for the next frame, reads it on the device, and opens the
    // check screen with the reading. The camera is stopped as soon as a frame is
    // in hand. A frame that cannot be read returns to `ready` with a problem and
    // the camera on again.
    Q_INVOKABLE void capture();

    // The typed-board fallback: an empty 4x4, straight to the check screen.
    Q_INVOKABLE void typeBoard();
    // The check screen's "Read again" and Back: the empty grid is dropped, the
    // camera starts again, and the screen is `ready`.
    Q_INVOKABLE void readAgain();

    // --- the check screen's corrections -------------------------------------
    Q_INVOKABLE bool setTile(int index, const QString& letters);
    Q_INVOKABLE bool acceptTile(int index);
    Q_INVOKABLE bool clearTile(int index);
    // A quarter turn, clockwise, to match how the child sees the real board.
    Q_INVOKABLE void turn();
    Q_INVOKABLE bool resize(int rows, int columns);
    // The reader's other thoughts for a tile, best first, to offer before A to Z.
    Q_INVOKABLE QStringList alternatives(int index) const;

    // "Yes, play", the grown-up's confirmation: the draft becomes a Board, and
    // only here. False, and nothing changed, while a tile is still empty.
    Q_INVOKABLE bool confirm();

    // The reading the camera path fills the draft with, as the reader gave it.
    // Public for the camera path and for the tests: a screen never calls it.
    bool useReading(const wjs::scan::BoardReading& reading);

    // The confirmed board, for a test to read what "Yes, play" made.
    [[nodiscard]] const std::optional<wjs::Board>& confirmedBoard() const noexcept {
        return confirmed_;
    }

signals:
    void stateChanged();
    void draftChanged();
    void cameraChanged();
    void readProblemChanged();
    void confirmedChanged();

private:
    enum class State { ready, reading, check };

    void setState(State state);
    void stopCamera();
    void tryFrame();
    void failed(const QString& code);
    void changedDraft();
    [[nodiscard]] bool on_grid(int index) const;

    State state_ = State::ready;
    std::optional<wjs::scan::BoardDraft> draft_;
    std::optional<wjs::Board> confirmed_;

    bool camera_available_ = false;
    bool camera_capturing_ = false;
    QString camera_problem_;
    QString read_problem_;
    QTimer frame_timer_;
    int frame_ticks_ = 0;
};

}  // namespace wjs::app
