#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <utility>
#include <vector>

#include "fake_solo_game.hpp"
#include "solo_game_factory.hpp"
#include "solo_model.hpp"

namespace {

using wjs::app::FakeSoloGame;
using wjs::app::SoloModel;

// Everything a screen is told, in order.
struct Heard {
    struct Traced {
        QString kind;
        QVariantList path;
        QString word;
        QString tier;
        int stars = 0;
        bool again = false;
        bool new_best = false;
        bool cleared = false;
    };
    struct Cue {
        QString kind;
        QString detail;
    };
    std::vector<Traced> traced;
    std::vector<Cue> cues;
    int state_changes = 0;
};

// A model over the fake game, with the touch geometry set: tiles of 100 and a gap
// of 10, so the tile at (row, column) has its centre at (50 + 110 c, 50 + 110 r).
struct Fixture {
    SoloModel model{std::make_unique<FakeSoloGame>()};
    Heard heard;

    Fixture() {
        model.setGeometry(100, 10);
        QObject::connect(
            &model, &SoloModel::traced,
            [this](const QString& kind, const QVariantList& path, const QString& word,
                   const QString& tier, int stars, bool again, bool new_best, bool cleared) {
                heard.traced.push_back({kind, path, word, tier, stars, again, new_best, cleared});
            });
        QObject::connect(&model, &SoloModel::cue,
                         [this](const QString& kind, const QString& detail) {
                             heard.cues.push_back({kind, detail});
                         });
        QObject::connect(&model, &SoloModel::stateChanged, [this] { ++heard.state_changes; });
    }

    // Into the box's shake and out of it, as the scene does.
    void play() {
        model.shake();
        model.sceneSettled();
    }

    static double x(int column) { return 50 + 110.0 * column; }
    static double y(int row) { return 50 + 110.0 * row; }

    // A finger down on a tile, sliding across the others in turn, and lifted.
    void trace(std::vector<std::pair<int, int>> tiles) {
        model.pressAt(x(tiles.front().second), y(tiles.front().first));
        for (std::size_t i = 1; i < tiles.size(); ++i) {
            const auto [row, column] = tiles[i];
            model.moveTo(x(column), y(row));
        }
        model.releaseAt(x(tiles.back().second), y(tiles.back().first), true);
    }

    // The tile of the first board as the model hands them to QML.
    //   E N S / O A I / M P T
    void sit() { trace({{0, 2}, {1, 2}, {2, 2}}); }
    void sat() { trace({{0, 2}, {1, 1}, {2, 2}}); }
    void tap() { trace({{2, 2}, {1, 1}, {2, 1}}); }
    void pat() { trace({{2, 1}, {1, 1}, {2, 2}}); }
    void nip() { trace({{0, 1}, {1, 2}, {2, 1}}); }

    // The presentation of a find is the screen's to time: it ends it.
    void shown() { model.wordShown(); }
};

QVariantList indices(std::initializer_list<int> values) {
    QVariantList list;
    for (const int value : values) {
        list.append(value);
    }
    return list;
}

}  // namespace

TEST_CASE("the model starts in the box with nothing on the table", "[app][solo]") {
    Fixture f;

    CHECK(f.model.state() == "box");
    CHECK(f.model.boardSize() == 3);
    CHECK(f.model.sizes() == indices({3, 4}));
    CHECK(f.model.tiles().isEmpty());
    CHECK(f.model.rows() == 0);
    CHECK(f.model.found().isEmpty());
    CHECK(f.model.history().isEmpty());
    CHECK(f.model.helpTile() == -1);
    CHECK(f.model.helpAllowed());
    CHECK(f.model.path().isEmpty());
}

TEST_CASE("the size is chosen in the box and tells the screen", "[app][solo]") {
    Fixture f;
    int changes = 0;
    QObject::connect(&f.model, &SoloModel::sizeChanged, [&] { ++changes; });

    f.model.chooseSize(4);
    CHECK(f.model.boardSize() == 4);
    CHECK(changes == 1);
    f.model.chooseSize(4);  // already: nothing to say
    f.model.chooseSize(7);
    CHECK(changes == 1);

    f.model.shake();
    f.model.chooseSize(3);
    CHECK(f.model.boardSize() == 4);
}

TEST_CASE("a shake deals the board and the scene's end of it starts play", "[app][solo]") {
    Fixture f;
    int dealt = 0;
    QObject::connect(&f.model, &SoloModel::boardChanged, [&] { ++dealt; });

    f.model.shake();

    CHECK(f.model.state() == "shaking");
    CHECK(f.model.rows() == 3);
    CHECK(f.model.columns() == 3);
    CHECK(f.model.tiles() == QStringList{"E", "N", "S", "O", "A", "I", "M", "P", "T"});
    CHECK(f.model.boardVersion() == 1);
    CHECK(dealt == 1);
    REQUIRE(f.heard.cues.size() == 1);
    CHECK(f.heard.cues[0].kind == "rattle");

    // A second shake does nothing; the scene's word moves the game on.
    f.model.shake();
    CHECK(f.model.boardVersion() == 1);
    f.model.sceneSettled();
    CHECK(f.model.state() == "playing");
    // And a late word from the scene is ignored.
    f.model.sceneSettled();
    CHECK(f.model.state() == "playing");
}

TEST_CASE("nothing is traced before the dice have settled", "[app][solo]") {
    Fixture f;
    f.model.shake();

    f.model.pressAt(Fixture::x(0), Fixture::y(0));
    f.model.releaseAt(Fixture::x(0), Fixture::y(0), true);
    f.model.tapTile(0);

    CHECK(f.model.path().isEmpty());
}

TEST_CASE("a viable word is shown, then the game goes on", "[app][solo]") {
    Fixture f;
    f.play();

    f.sit();

    REQUIRE(f.heard.traced.size() == 1);
    const auto& traced = f.heard.traced[0];
    CHECK(traced.kind == "word");
    CHECK(traced.word == "sit");
    CHECK(traced.tier == "spark");
    CHECK(traced.stars == 1);
    CHECK_FALSE(traced.again);
    CHECK_FALSE(traced.cleared);

    // solo.word-found: the word's path is lit and the path in the making is gone.
    CHECK(f.model.state() == "word-found");
    CHECK(f.model.litPath() == indices({2, 5, 8}));
    CHECK(f.model.path().isEmpty());
    REQUIRE(f.model.found().size() == 1);
    const auto jewel = f.model.found().at(0).toMap();
    CHECK(jewel.value("word").toString() == "sit");
    CHECK(jewel.value("tier").toString() == "spark");
    CHECK(jewel.value("stars").toInt() == 1);

    // The sound of the word is a cue, and the tile letters came first, in order.
    REQUIRE(f.heard.cues.size() >= 5);
    CHECK(f.heard.cues[1].kind == "tile");
    CHECK(f.heard.cues[1].detail == "S");
    CHECK(f.heard.cues[2].detail == "I");
    CHECK(f.heard.cues[3].detail == "T");
    CHECK(f.heard.cues.back().kind == "word");
    CHECK(f.heard.cues.back().detail == "sit");

    // While it is shown the board does not take a new path.
    f.model.pressAt(Fixture::x(0), Fixture::y(0));
    CHECK(f.model.path().isEmpty());

    f.shown();
    CHECK(f.model.state() == "playing");
    CHECK(f.model.litPath().isEmpty());
}

TEST_CASE("a real word that is not today's gets a soft tick and no word", "[app][solo]") {
    Fixture f;
    f.play();

    // TIP, down the right-hand column and across.
    f.trace({{2, 2}, {1, 2}, {2, 1}});

    REQUIRE(f.heard.traced.size() == 1);
    const auto& traced = f.heard.traced[0];
    CHECK(traced.kind == "tick");
    // Nothing the screen is told can show or read the word.
    CHECK(traced.word.isEmpty());
    CHECK(traced.tier.isEmpty());
    CHECK(traced.stars == 0);
    CHECK(f.model.found().isEmpty());
    CHECK(f.model.state() == "playing");
    CHECK(f.heard.cues.back().kind == "tick");
    CHECK(f.heard.cues.back().detail.isEmpty());
}

TEST_CASE("anything else fades with no sound and no word", "[app][solo]") {
    Fixture f;
    f.play();
    const auto cues = f.heard.cues.size();
    const auto tile_cues = 2;  // the two tiles joined

    f.trace({{0, 0}, {0, 1}});

    REQUIRE(f.heard.traced.size() == 1);
    CHECK(f.heard.traced[0].kind == "fade");
    CHECK(f.heard.traced[0].word.isEmpty());
    CHECK(f.heard.traced[0].path == indices({0, 1}));
    // The path's tiles sounded as they joined and nothing else did.
    CHECK(f.heard.cues.size() == cues + tile_cues);
    CHECK(f.model.found().isEmpty());
}

TEST_CASE("tapping in order and pressing Done hands a word in", "[app][solo]") {
    Fixture f;
    f.play();

    f.model.tapTile(2);
    f.model.tapTile(5);
    f.model.tapTile(8);
    CHECK(f.model.path() == indices({2, 5, 8}));
    CHECK_FALSE(f.model.pointerDown());
    CHECK(f.heard.traced.empty());

    f.model.done();

    REQUIRE(f.heard.traced.size() == 1);
    CHECK(f.heard.traced[0].word == "sit");
    CHECK(f.model.path().isEmpty());
}

TEST_CASE("tapping the last tile again hands a word in, and Escape clears", "[app][solo]") {
    Fixture f;
    f.play();

    f.model.tapTile(2);
    f.model.tapTile(5);
    f.model.tapTile(8);
    f.model.tapTile(8);
    CHECK(f.heard.traced.size() == 1);

    f.shown();
    f.model.tapTile(2);
    CHECK_FALSE(f.model.path().isEmpty());
    f.model.clearPath();
    CHECK(f.model.path().isEmpty());
    CHECK(f.heard.traced.size() == 1);
}

TEST_CASE("a word found again is shown again with no new jewel", "[app][solo]") {
    Fixture f;
    f.play();
    f.sit();
    f.shown();

    f.sit();

    REQUIRE(f.heard.traced.size() == 2);
    CHECK(f.heard.traced[1].again);
    CHECK(f.model.found().size() == 1);
}

TEST_CASE("the start tile is one press, and it goes out when its word is found", "[app][solo]") {
    Fixture f;
    f.play();
    int changes = 0;
    QObject::connect(&f.model, &SoloModel::helpChanged, [&] { ++changes; });

    // Nothing lights by itself.
    CHECK(f.model.helpTile() == -1);

    f.model.askHelp();
    CHECK(f.model.helpTile() == 1);  // N, row 0 column 1: NIP
    CHECK(changes == 1);

    f.sit();
    f.shown();
    CHECK(f.model.helpTile() == 1);

    f.nip();
    f.shown();
    CHECK(f.model.helpTile() == -1);
}

TEST_CASE("the parent's switch takes the start tile away", "[app][solo]") {
    auto game = std::make_unique<FakeSoloGame>();
    game->setHelpAllowed(false);
    SoloModel model{std::move(game)};
    model.shake();
    model.sceneSettled();

    CHECK_FALSE(model.helpAllowed());
    model.askHelp();
    CHECK(model.helpTile() == -1);
}

TEST_CASE("tiles that make one sound are one block as the path grows", "[app][solo]") {
    Fixture f;
    f.play();

    f.model.pressAt(Fixture::x(1), Fixture::y(1));  // A
    f.model.moveTo(Fixture::x(2), Fixture::y(1));   // I
    const auto blocks = f.model.blocks();

    REQUIRE(blocks.size() == 1);
    const auto block = blocks.at(0).toMap();
    CHECK(block.value("first").toInt() == 0);
    CHECK(block.value("tiles").toInt() == 2);
    CHECK(block.value("grapheme").toString() == "ai");
    CHECK(f.model.pointerDown());

    f.model.releaseAt(Fixture::x(2), Fixture::y(1), false);
    CHECK(f.model.path().isEmpty());
    CHECK(f.model.blocks().isEmpty());
}

TEST_CASE("a dead jewel cannot be touched", "[app][solo]") {
    Fixture f;
    f.model.chooseSize(4);
    f.model.shake();
    f.model.sceneSettled();
    f.model.setGeometry(100, 10);

    const auto tiles = f.model.tiles();
    const int dead = static_cast<int>(tiles.indexOf(QString{}));
    REQUIRE(dead >= 0);
    const int row = dead / 4;
    const int column = dead % 4;
    f.model.pressAt(Fixture::x(column), Fixture::y(row));
    f.model.releaseAt(Fixture::x(column), Fixture::y(row), true);
    f.model.tapTile(dead);

    CHECK(f.model.path().isEmpty());
}

TEST_CASE("the last word clears the board, which celebrates once and then rests", "[app][solo]") {
    Fixture f;
    f.play();
    for (auto word : {&Fixture::sit, &Fixture::sat, &Fixture::tap, &Fixture::pat}) {
        (f.*word)();
        f.shown();
        CHECK(f.model.state() == "playing");
    }

    f.nip();

    REQUIRE(f.heard.traced.size() == 5);
    CHECK(f.heard.traced.back().cleared);
    CHECK(f.heard.traced.back().new_best);
    // The word is shown first; the quiet celebration follows it.
    CHECK(f.model.state() == "word-found");
    f.shown();
    CHECK(f.model.state() == "cleared");
    CHECK(f.heard.cues.back().kind == "cleared");

    // The celebration cannot be cut short from outside its own end.
    f.model.celebrated();
    CHECK(f.model.state() == "resting");

    // Resting saves the board to history, which the child may open.
    REQUIRE(f.model.history().size() == 1);
    CHECK(f.model.history().at(0).toMap().value("found").toList().size() == 5);

    // Nothing on the resting screen comes back by itself: only Again leaves it.
    f.model.shake();
    CHECK(f.model.state() == "resting");
    f.model.again();
    CHECK(f.model.state() == "box");
}

TEST_CASE("putting the lid back on saves the board and clears what was in the making",
          "[app][solo]") {
    Fixture f;
    f.play();
    f.sit();
    f.shown();
    f.model.askHelp();
    f.model.tapTile(0);

    f.model.lidOn();

    CHECK(f.model.state() == "box");
    CHECK(f.model.path().isEmpty());
    CHECK(f.model.helpTile() == -1);
    REQUIRE(f.model.history().size() == 1);
    const auto entry = f.model.history().at(0).toMap();
    CHECK(entry.value("rows").toInt() == 3);
    CHECK(entry.value("tiles").toStringList().size() == 9);
    CHECK(entry.value("found").toList().size() == 1);
}

TEST_CASE("the lid can go back on mid-shake, and nothing is saved", "[app][solo]") {
    Fixture f;
    f.model.shake();

    f.model.lidOn();

    CHECK(f.model.state() == "box");
    CHECK(f.model.history().isEmpty());
}

TEST_CASE("a model made with no game plays the one the application plays", "[app][solo]") {
    SoloModel model;

    CHECK(model.state() == "box");
    CHECK(model.sizes() == indices({3, 4}));
    model.shake();
    CHECK(model.state() == "shaking");
}

// --- what the parent changes -------------------------------------------------------

namespace {

// What a program's maker makes after the parent switched the help off and chose the
// 4x4 to be offered first, and how many it has made.
struct ParentChoices {
    int made = 0;
    wjs::app::ScopedSoloGameMaker installed{[this] {
        ++made;
        auto game = std::make_unique<FakeSoloGame>();
        game->setHelpAllowed(false);
        game->chooseSize(4);
        return game;
    }};
};

}  // namespace

TEST_CASE("an edit by the parent gives a new game at once when the box has its lid on",
          "[app][solo][edited]") {
    ParentChoices chosen;
    Fixture f;
    f.play();
    f.model.lidOn();
    REQUIRE(f.model.state() == "box");
    CHECK(f.model.helpAllowed());
    CHECK(f.model.boardSize() == 3);
    int states = 0;
    QObject::connect(&f.model, &SoloModel::stateChanged, [&] { ++states; });

    f.model.storeEdited();

    CHECK(chosen.made == 1);
    CHECK_FALSE(f.model.helpAllowed());
    CHECK(f.model.boardSize() == 4);
    CHECK(f.model.state() == "box");
    CHECK(f.model.tiles().isEmpty());
    CHECK(f.model.found().isEmpty());
    CHECK(states == 1);
    // And it plays like any other.
    f.model.shake();
    CHECK(f.model.state() == "shaking");
    CHECK(f.model.rows() == 4);
}

TEST_CASE("a game in play is not taken from a child: the next one is made when the lid is on",
          "[app][solo][edited]") {
    ParentChoices chosen;
    Fixture f;
    f.play();
    const QStringList tiles = f.model.tiles();

    f.model.storeEdited();

    CHECK(chosen.made == 0);
    CHECK(f.model.state() == "playing");
    CHECK(f.model.tiles() == tiles);
    CHECK(f.model.helpAllowed());

    f.model.lidOn();

    CHECK(chosen.made == 1);
    CHECK(f.model.state() == "box");
    CHECK_FALSE(f.model.helpAllowed());
}

TEST_CASE("a game that is resting is replaced by Again", "[app][solo][edited]") {
    ParentChoices chosen;
    Fixture f;
    f.play();
    for (auto word : {&Fixture::sit, &Fixture::sat, &Fixture::tap, &Fixture::pat, &Fixture::nip}) {
        (f.*word)();
        f.shown();
    }
    f.model.celebrated();
    REQUIRE(f.model.state() == "resting");

    f.model.storeEdited();
    CHECK(chosen.made == 0);
    CHECK(f.model.state() == "resting");

    f.model.again();
    CHECK(chosen.made == 1);
    CHECK(f.model.state() == "box");
    CHECK_FALSE(f.model.helpAllowed());
}

TEST_CASE("with no edit the game goes on as it is, whatever the maker would make",
          "[app][solo][edited]") {
    ParentChoices chosen;
    Fixture f;
    f.play();
    f.model.lidOn();
    f.model.shake();
    f.model.lidOn();

    CHECK(chosen.made == 0);
    CHECK(f.model.helpAllowed());
}

TEST_CASE("a new game that was brought back is shown as the constructor shows one",
          "[app][solo][edited]") {
    int made = 0;
    const wjs::app::ScopedSoloGameMaker installed([&made] {
        ++made;
        auto game = std::make_unique<FakeSoloGame>();
        game->shake();
        game->settled();
        return game;
    });
    Fixture f;
    const int version = f.model.boardVersion();

    f.model.storeEdited();

    CHECK(made == 1);
    CHECK(f.model.state() == "playing");
    CHECK(f.model.rows() == 3);
    CHECK(f.model.tiles().size() == 9);
    CHECK(f.model.boardVersion() == version + 1);
}
