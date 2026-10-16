#pragma once

#include <QtQml/qqmlregistration.h>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <memory>

#include "path_tracer.hpp"
#include "solo_game.hpp"

namespace wjs::app {

// What the Solo Play screens bind to: the game's state, board, tray and path as
// Qt Quick wants them, and the touch input turned into moves. It decides
// nothing about words (the SoloGame behind it does) and plays nothing: every
// moment that wants a sound or a tap of the hand is a `cue`, which no sound
// code listens to yet.
//
// It owns three things the game does not:
//   - the presentation state `word-found` (docs/design/game.md), the few moments
//     after a find when the word is shown, which the screens end with wordShown();
//   - the path being traced or tapped (PathTracer), in tile indices;
//   - the sound-it-out blocks of that path, as the finger moves.
//
// Tile indices are row * columns + column, from the top left.
class SoloModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    // "box", "shaking", "dropping", "playing", "word-found", "cleared" or
    // "resting": the ids of docs/design/game.md without the "solo." prefix.
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)

    // The size chosen (also in the box), and those offered.
    Q_PROPERTY(int boardSize READ boardSize NOTIFY sizeChanged)
    Q_PROPERTY(QVariantList sizes READ sizes CONSTANT)

    // The board on the table: empty in the box until the first shake. A dead
    // jewel is "". `boardVersion` is a new number for each board dealt.
    Q_PROPERTY(int rows READ rows NOTIFY boardChanged)
    Q_PROPERTY(int columns READ columns NOTIFY boardChanged)
    Q_PROPERTY(QStringList tiles READ tiles NOTIFY boardChanged)
    Q_PROPERTY(uint seed READ seed NOTIFY boardChanged)
    Q_PROPERTY(int boardVersion READ boardVersion NOTIFY boardChanged)

    // The path being made, as tile indices, and its grapheme blocks, one map
    // each: first (index in the path), tiles (how many), grapheme (letters).
    Q_PROPERTY(QVariantList path READ path NOTIFY pathChanged)
    Q_PROPERTY(QVariantList blocks READ blocks NOTIFY pathChanged)
    Q_PROPERTY(bool pointerDown READ pointerDown NOTIFY pathChanged)
    // The path of the word being shown, in `word-found`.
    Q_PROPERTY(QVariantList litPath READ litPath NOTIFY litPathChanged)

    // The tray: one map for each word found on this board: word (lower-case),
    // tier ("spark", "power", "mega") and stars.
    Q_PROPERTY(QVariantList found READ found NOTIFY foundChanged)

    // The help: whether the parent allows it, and the tile lit now (-1: none).
    Q_PROPERTY(bool helpAllowed READ helpAllowed NOTIFY helpChanged)
    Q_PROPERTY(int helpTile READ helpTile NOTIFY helpChanged)

    // Earlier boards, newest first: rows, columns, tiles and found (as the tray's).
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)

public:
    explicit SoloModel(QObject* parent = nullptr);
    explicit SoloModel(std::unique_ptr<SoloGame> game, QObject* parent = nullptr);
    ~SoloModel() override;

    [[nodiscard]] QString state() const;
    [[nodiscard]] int boardSize() const;
    [[nodiscard]] QVariantList sizes() const;
    [[nodiscard]] int rows() const;
    [[nodiscard]] int columns() const;
    [[nodiscard]] QStringList tiles() const;
    [[nodiscard]] uint seed() const;
    [[nodiscard]] int boardVersion() const { return board_version_; }
    [[nodiscard]] QVariantList path() const;
    [[nodiscard]] QVariantList blocks() const;
    [[nodiscard]] bool pointerDown() const { return tracer_.pointerDown(); }
    [[nodiscard]] QVariantList litPath() const { return lit_path_; }
    [[nodiscard]] QVariantList found() const;
    [[nodiscard]] bool helpAllowed() const;
    [[nodiscard]] int helpTile() const;
    [[nodiscard]] QVariantList history() const { return history_; }

    // --- the box
    Q_INVOKABLE void chooseSize(int size);
    // The Shake button, Space, or a device shake. Does nothing outside the box.
    Q_INVOKABLE void shake();
    // The board scene says its dice are still.
    Q_INVOKABLE void sceneSettled();

    // --- making a word. Positions are in pixels from the board's top left.
    Q_INVOKABLE void setGeometry(qreal tile, qreal gap);
    Q_INVOKABLE void pressAt(qreal x, qreal y);
    Q_INVOKABLE void moveTo(qreal x, qreal y);
    Q_INVOKABLE void releaseAt(qreal x, qreal y, bool inside);
    // Input with no pointer: Space on the focused tile, a screen reader's
    // activation. The same as tapping the tile.
    Q_INVOKABLE void tapTile(int index);
    // The Done jewel, Enter.
    Q_INVOKABLE void done();
    Q_INVOKABLE void removeLast();
    Q_INVOKABLE void clearPath();

    // --- the moments the screens time
    // The word has been shown: -> playing, or -> cleared after the last one.
    Q_INVOKABLE void wordShown();
    // The quiet celebration is over: -> resting.
    Q_INVOKABLE void celebrated();

    // --- help, and the ways out
    Q_INVOKABLE void askHelp();
    // Back from playing: the lid goes on, the board is saved.
    Q_INVOKABLE void lidOn();
    // Again from resting, or leaving it for home: the box is ready next time.
    Q_INVOKABLE void again();

signals:
    void stateChanged();
    void sizeChanged();
    void boardChanged();
    void pathChanged();
    void litPathChanged();
    void foundChanged();
    void helpChanged();
    void historyChanged();

    // A path was handed in. `kind` is "word" (a viable word: `word`, `tier` and
    // `stars` say which; `again` is true when it was on the tray already), "tick"
    // (a real word that is not today's: no word is given), or "fade" (anything
    // else). `path` is the tiles, for the fade and the tick. `newBest` and
    // `cleared` come with the last word.
    void traced(const QString& kind, const QVariantList& path, const QString& word,
                const QString& tier, int stars, bool again, bool newBest, bool cleared);

    // A moment that wants a sound or a tap of the hand: "rattle", "tile" (detail:
    // the tile's letters, for its sound), "word" (the word), "tick", "chime",
    // "cleared". Nothing listens yet (docs/design/application.md, "Audio and
    // touch feedback"); the cues are where it will.
    void cue(const QString& kind, const QString& detail);

private:
    void applyEvent(PathTracer::Event event);
    void submitPath();
    void resetTracer();
    void refreshHistory();
    void refreshHelp();
    [[nodiscard]] QVariantList indices(std::span<const Position> path) const;
    [[nodiscard]] QString lettersAt(Position position) const;

    std::unique_ptr<SoloGame> game_;
    PathTracer tracer_;
    bool presenting_ = false;  // solo.word-found
    int board_version_ = 0;
    int help_tile_ = -1;
    QString last_cue_tile_;
    QVariantList lit_path_;
    QVariantList history_;
    std::size_t last_path_size_ = 0;
};

}  // namespace wjs::app
