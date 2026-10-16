#include "solo_model.hpp"

#include <QVariantMap>
#include <utility>

#include "solo_game_factory.hpp"

namespace wjs::app {
namespace {

[[nodiscard]] QString to_qstring(const std::string& text) {
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

[[nodiscard]] QString tier_name(WordTier tier) {
    switch (tier) {
        case WordTier::spark:
            return QStringLiteral("spark");
        case WordTier::power:
            return QStringLiteral("power");
        case WordTier::mega:
            return QStringLiteral("mega");
    }
    return QStringLiteral("spark");
}

[[nodiscard]] QVariantMap found_map(const SoloFound& found) {
    QVariantMap map;
    map.insert(QStringLiteral("word"), to_qstring(found.word));
    map.insert(QStringLiteral("tier"), tier_name(found.tier));
    map.insert(QStringLiteral("stars"), found.stars);
    return map;
}

[[nodiscard]] QStringList tile_list(const SoloBoard& board) {
    QStringList tiles;
    tiles.reserve(static_cast<qsizetype>(board.tiles.size()));
    for (const auto& tile : board.tiles) {
        tiles.append(to_qstring(tile));
    }
    return tiles;
}

}  // namespace

SoloModel::SoloModel(QObject* parent) : SoloModel(make_solo_game(), parent) {}

SoloModel::SoloModel(std::unique_ptr<SoloGame> game, QObject* parent)
    : QObject(parent), game_(std::move(game)) {
    refreshHistory();
}

SoloModel::~SoloModel() = default;

QString SoloModel::state() const {
    if (presenting_) {
        return QStringLiteral("word-found");
    }
    const auto name = to_string(game_->state());
    return QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size()));
}

int SoloModel::boardSize() const {
    return static_cast<int>(game_->size());
}

QVariantList SoloModel::sizes() const {
    QVariantList list;
    for (const std::size_t size : game_->sizes()) {
        list.append(static_cast<int>(size));
    }
    return list;
}

int SoloModel::rows() const {
    return static_cast<int>(game_->board().rows);
}

int SoloModel::columns() const {
    return static_cast<int>(game_->board().columns);
}

QStringList SoloModel::tiles() const {
    return tile_list(game_->board());
}

uint SoloModel::seed() const {
    return game_->board().seed;
}

QVariantList SoloModel::indices(std::span<const Position> path) const {
    QVariantList list;
    const std::size_t columns = game_->board().columns;
    for (const auto& position : path) {
        list.append(static_cast<int>(position.row * columns + position.column));
    }
    return list;
}

QString SoloModel::lettersAt(Position position) const {
    const auto& board = game_->board();
    if (position.row >= board.rows || position.column >= board.columns) {
        return {};
    }
    return to_qstring(board.tile(position));
}

QVariantList SoloModel::path() const {
    return indices(tracer_.path());
}

QVariantList SoloModel::blocks() const {
    QVariantList list;
    if (tracer_.path().empty()) {
        return list;
    }
    for (const auto& block : game_->blocks(tracer_.path())) {
        QVariantMap map;
        map.insert(QStringLiteral("first"), static_cast<int>(block.first));
        map.insert(QStringLiteral("tiles"), static_cast<int>(block.tiles));
        map.insert(QStringLiteral("grapheme"), to_qstring(block.grapheme));
        list.append(map);
    }
    return list;
}

QVariantList SoloModel::found() const {
    QVariantList list;
    for (const auto& entry : game_->found()) {
        list.append(found_map(entry));
    }
    return list;
}

bool SoloModel::helpAllowed() const {
    return game_->helpAllowed();
}

int SoloModel::helpTile() const {
    return help_tile_;
}

void SoloModel::refreshHistory() {
    QVariantList list;
    for (const auto& entry : game_->history()) {
        QVariantMap map;
        map.insert(QStringLiteral("rows"), static_cast<int>(entry.board.rows));
        map.insert(QStringLiteral("columns"), static_cast<int>(entry.board.columns));
        map.insert(QStringLiteral("tiles"), tile_list(entry.board));
        QVariantList words;
        for (const auto& found : entry.found) {
            words.append(found_map(found));
        }
        map.insert(QStringLiteral("found"), words);
        list.append(map);
    }
    history_ = std::move(list);
    emit historyChanged();
}

void SoloModel::refreshHelp() {
    int tile = -1;
    if (const auto lit = game_->litTile()) {
        tile = static_cast<int>(lit->row * game_->board().columns + lit->column);
    }
    if (tile != help_tile_) {
        help_tile_ = tile;
        emit helpChanged();
    }
}

void SoloModel::resetTracer() {
    const auto& board = game_->board();
    std::vector<bool> usable;
    usable.reserve(board.tiles.size());
    for (const auto& tile : board.tiles) {
        usable.push_back(!tile.empty());
    }
    tracer_.setBoard(board.rows, board.columns, std::move(usable));
    last_path_size_ = 0;
    emit pathChanged();
}

// --- the box ----------------------------------------------------------------

void SoloModel::chooseSize(int size) {
    if (size < 0 || game_->state() != SoloState::box) {
        return;
    }
    const auto before = game_->size();
    game_->chooseSize(static_cast<std::size_t>(size));
    if (game_->size() != before) {
        emit sizeChanged();
    }
}

void SoloModel::shake() {
    if (game_->state() != SoloState::box || !game_->shake()) {
        return;
    }
    presenting_ = false;
    lit_path_.clear();
    help_tile_ = -1;
    ++board_version_;
    resetTracer();
    emit boardChanged();
    emit foundChanged();
    emit helpChanged();
    emit litPathChanged();
    emit stateChanged();
    emit cue(QStringLiteral("rattle"), {});
}

void SoloModel::sceneSettled() {
    if (game_->state() != SoloState::shaking && game_->state() != SoloState::dropping) {
        return;
    }
    game_->settled();
    emit stateChanged();
}

// --- making a word ------------------------------------------------------------

void SoloModel::setGeometry(qreal tile, qreal gap) {
    tracer_.setGeometry(tile, gap);
}

void SoloModel::applyEvent(PathTracer::Event event) {
    switch (event) {
        case PathTracer::Event::none:
            return;
        case PathTracer::Event::changed:
        case PathTracer::Event::cancelled: {
            // Each tile that joined sounds its letter (a sound is cut off by the next).
            const auto size = tracer_.path().size();
            for (std::size_t i = last_path_size_; i < size; ++i) {
                emit cue(QStringLiteral("tile"), lettersAt(tracer_.path()[i]));
            }
            last_path_size_ = size;
            emit pathChanged();
            return;
        }
        case PathTracer::Event::submit:
            submitPath();
            return;
    }
}

void SoloModel::submitPath() {
    const std::vector<Position> path = tracer_.path();
    const QVariantList traced_path = indices(path);
    tracer_.clear();
    last_path_size_ = 0;
    emit pathChanged();

    const auto outcome = game_->submit(path);
    switch (outcome.kind) {
        case WordKind::viable: {
            presenting_ = true;
            lit_path_ = traced_path;
            emit litPathChanged();
            emit foundChanged();
            refreshHelp();
            emit stateChanged();
            emit traced(QStringLiteral("word"), traced_path, to_qstring(outcome.found.word),
                        tier_name(outcome.found.tier), outcome.found.stars, outcome.alreadyFound,
                        outcome.newBest, outcome.cleared);
            emit cue(QStringLiteral("word"), to_qstring(outcome.found.word));
            if (outcome.newBest) {
                emit cue(QStringLiteral("chime"), {});
            }
            return;
        }
        case WordKind::realNotTodays:
            // A real word that is not today's: no word anywhere in the outcome.
            emit traced(QStringLiteral("tick"), traced_path, {}, {}, 0, false, false, false);
            emit cue(QStringLiteral("tick"), {});
            return;
        case WordKind::notAWord:
            emit traced(QStringLiteral("fade"), traced_path, {}, {}, 0, false, false, false);
            return;
    }
}

void SoloModel::pressAt(qreal x, qreal y) {
    if (game_->state() != SoloState::playing || presenting_) {
        return;
    }
    applyEvent(tracer_.press({x, y}));
    emit pathChanged();
}

void SoloModel::moveTo(qreal x, qreal y) {
    if (game_->state() != SoloState::playing || presenting_) {
        return;
    }
    applyEvent(tracer_.move({x, y}));
}

void SoloModel::releaseAt(qreal x, qreal y, bool inside) {
    if (game_->state() != SoloState::playing || presenting_) {
        // A finger lifted over a word being shown still lets go of the tracer.
        applyEvent(tracer_.release({x, y}, false));
        return;
    }
    applyEvent(tracer_.release({x, y}, inside));
    emit pathChanged();
}

void SoloModel::tapTile(int index) {
    const auto& board = game_->board();
    if (index < 0 || board.columns == 0 || game_->state() != SoloState::playing || presenting_) {
        return;
    }
    const auto i = static_cast<std::size_t>(index);
    applyEvent(tracer_.tap({i / board.columns, i % board.columns}));
}

void SoloModel::done() {
    if (game_->state() != SoloState::playing || presenting_) {
        return;
    }
    applyEvent(tracer_.submit());
}

void SoloModel::removeLast() {
    applyEvent(tracer_.removeLast());
}

void SoloModel::clearPath() {
    applyEvent(tracer_.clear());
}

// --- the moments the screens time ----------------------------------------------

void SoloModel::wordShown() {
    if (!presenting_) {
        return;
    }
    presenting_ = false;
    lit_path_.clear();
    emit litPathChanged();
    emit stateChanged();
    if (game_->state() == SoloState::cleared) {
        emit cue(QStringLiteral("cleared"), {});
    }
}

void SoloModel::celebrated() {
    if (game_->state() != SoloState::cleared || presenting_) {
        return;
    }
    game_->celebrated();
    refreshHistory();
    emit stateChanged();
}

// --- help, and the ways out ----------------------------------------------------

void SoloModel::askHelp() {
    if (game_->state() != SoloState::playing || presenting_ || !game_->helpAllowed()) {
        return;
    }
    game_->startTile();
    refreshHelp();
}

void SoloModel::lidOn() {
    if (game_->state() != SoloState::playing && game_->state() != SoloState::shaking &&
        game_->state() != SoloState::dropping) {
        return;
    }
    presenting_ = false;
    lit_path_.clear();
    help_tile_ = -1;
    tracer_.clear();
    game_->lidOn();
    refreshHistory();
    emit pathChanged();
    emit litPathChanged();
    emit helpChanged();
    emit stateChanged();
    renewGameIfDue();
}

void SoloModel::again() {
    if (game_->state() != SoloState::resting) {
        return;
    }
    presenting_ = false;
    help_tile_ = -1;
    game_->again();
    refreshHistory();
    emit helpChanged();
    emit stateChanged();
    renewGameIfDue();
}

// --- what the parent changes ----------------------------------------------------

void SoloModel::storeEdited() {
    renew_due_ = true;
    renewGameIfDue();
}

void SoloModel::renewGameIfDue() {
    if (!renew_due_ || game_->state() != SoloState::box) {
        return;
    }
    renew_due_ = false;
    // The game is made from the store as it is now (the maker is the program's), and
    // the box is shown as a new game's: empty, with the size and the help the parent
    // has chosen and the history the store keeps.
    game_ = make_solo_game();
    presenting_ = false;
    help_tile_ = -1;
    lit_path_.clear();
    resetTracer();
    refreshHistory();
    emit sizeChanged();
    emit boardChanged();
    emit foundChanged();
    emit helpChanged();
    emit litPathChanged();
    emit stateChanged();
}

}  // namespace wjs::app
