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
//
// It starts where the game is. Over a new game that is the box. Over one brought
// back from the run before it is `playing`, with the board, the tray and the lit
// tile as they were left and nothing to shake, or `resting`: the screens open on
// it, and the scene is given the board with `show` and not `play`.
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
    // jewel is "". `faces` is the six faces of the die on each tile, row-major,
    // and `up` is which of them is up, for the scene to tumble them; both are
    // empty when the board was not rolled from dice (the phonics board, a board
    // brought back from the store, the fake game). `boardVersion` is a new number
    // for each board dealt.
    Q_PROPERTY(int rows READ rows NOTIFY boardChanged)
    Q_PROPERTY(int columns READ columns NOTIFY boardChanged)
    Q_PROPERTY(QStringList tiles READ tiles NOTIFY boardChanged)
    Q_PROPERTY(QVariantList faces READ faces NOTIFY boardChanged)
    Q_PROPERTY(QVariantList up READ up NOTIFY boardChanged)
    Q_PROPERTY(uint seed READ seed NOTIFY boardChanged)
    Q_PROPERTY(int boardVersion READ boardVersion NOTIFY boardChanged)

    // The path being made, as tile indices, and its grapheme blocks, one map
    // each: first (index in the path), tiles (how many), grapheme (letters).
    // The lists are built once per change and read many times (the screens read
    // `path` once a tile and once a path line), so the getters return a cached
    // list and `pathChanged` means "the path changed": a rebuild per read is the
    // pointer's jank. `pointerDown` has its own signal so that a finger going
    // down or up does not rebuild the path's display.
    Q_PROPERTY(QVariantList path READ path NOTIFY pathChanged)
    Q_PROPERTY(QVariantList blocks READ blocks NOTIFY pathChanged)
    Q_PROPERTY(bool pointerDown READ pointerDown NOTIFY pointerDownChanged)
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
    [[nodiscard]] QVariantList faces() const;
    [[nodiscard]] QVariantList up() const;
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

    // --- what the parent changes
    // The parent changed a setting or reset what is kept (ParentController::
    // storeEdited). The next game is made from the store as it is now: a new one
    // at once when the box has its lid on, as the parent area is reached from it,
    // and otherwise when it next has. A game in play is never taken from a child.
    Q_INVOKABLE void storeEdited();

signals:
    void stateChanged();
    void sizeChanged();
    void boardChanged();
    void pathChanged();
    // The finger went down or up. Apart from `pathChanged` on purpose: the path
    // itself did not change, and the screens redraw it only for the path.
    void pointerDownChanged();
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
    void renewGameIfDue();
    [[nodiscard]] QVariantList indices(std::span<const Position> path) const;
    [[nodiscard]] QString lettersAt(Position position) const;
    // The cache behind the change signals: a flag is set where the signal is
    // sent, and the getter builds the list the first time it is read after
    // that. Unchanged reads return the same list, so a binding storm over
    // `path` costs one build, not one a read.
    void markPathChanged();
    void markBoardChanged();
    void markFoundChanged();

    std::unique_ptr<SoloGame> game_;
    PathTracer tracer_;
    bool presenting_ = false;  // solo.word-found
    bool renew_due_ = false;   // the store was edited since the game was made
    int board_version_ = 0;
    int help_tile_ = -1;
    QString last_cue_tile_;
    QVariantList lit_path_;
    QVariantList history_;
    std::size_t last_path_size_ = 0;

    mutable QVariantList path_cache_;
    mutable bool path_dirty_ = true;
    mutable QVariantList blocks_cache_;
    mutable bool blocks_dirty_ = true;
    mutable QStringList tiles_cache_;
    mutable QVariantList faces_cache_;
    mutable QVariantList up_cache_;
    mutable bool board_dirty_ = true;
    mutable QVariantList found_cache_;
    mutable bool found_dirty_ = true;
};

}  // namespace wjs::app
