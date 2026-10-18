#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>

#include "scan_model.hpp"
#include "wjs/scan/reading.hpp"

// The check screen's model (src/app/scan/scan_model.*): the draft a grown-up
// corrects, the reader's doubts and alternatives, the board size, the typed-board
// fallback, and the confirmation that is the only way to a Board. The camera
// itself is the platform's and is not faked here; on every desktop in this tree
// there is none, and the model says so with the typed board beside it.

namespace {

using wjs::app::ScanModel;

QStringList letters_of(const ScanModel& model) {
    return model.letters();
}

QVariantList indices(std::initializer_list<int> values) {
    QVariantList list;
    for (const int value : values) {
        list.append(value);
    }
    return list;
}

void fill(ScanModel& model, const QString& letter) {
    for (int index = 0; index < model.rows() * model.columns(); ++index) {
        REQUIRE(model.setTile(index, letter));
    }
}

wjs::scan::BoardReading two_by_two() {
    wjs::scan::BoardReading reading;
    reading.rows = 2;
    reading.columns = 2;
    reading.tiles.resize(4);
    reading.tiles[0] = {};
    reading.tiles[0].letters = "C";
    reading.tiles[0].confidence = 0.9;
    reading.tiles[0].doubtful = false;
    reading.tiles[1] = {};
    reading.tiles[1].letters = "A";
    reading.tiles[1].confidence = 0.4;
    reading.tiles[1].doubtful = true;
    reading.tiles[1].alternatives[0].letters = "O";
    reading.tiles[1].alternatives[0].score = 0.4;
    reading.tiles[2] = {};  // the reader saw nothing here
    reading.tiles[2].doubtful = true;
    reading.tiles[3] = {};
    reading.tiles[3].letters = "T";
    reading.tiles[3].confidence = 0.5;
    reading.tiles[3].doubtful = true;
    return reading;
}

}  // namespace

TEST_CASE("the typed board is an empty 4x4 the grown-up fills", "[app][scan]") {
    ScanModel model;
    CHECK(model.state() == "ready");
    CHECK(model.rows() == 0);
    CHECK(model.columns() == 0);

    model.typeBoard();

    CHECK(model.state() == "check");
    CHECK(model.rows() == 4);
    CHECK(model.columns() == 4);
    CHECK(model.size() == 4);
    CHECK(model.sizes().size() == 4);
    CHECK(model.letters().size() == 16);
    CHECK_FALSE(model.complete());
    CHECK(model.doubtful().isEmpty());
    for (const QString& letter : model.letters()) {
        CHECK(letter.isEmpty());
    }
    // The first tile is empty and was typed into, not read.
    const QVariantMap first = model.tiles().at(0).toMap();
    CHECK(first.value("letters").toString().isEmpty());
    CHECK(first.value("origin").toString() == "empty");
    CHECK_FALSE(first.value("doubtful").toBool());

    // A correction: one to three letters, upper-cased, and the tile settled.
    CHECK(model.setTile(0, "c"));
    CHECK(model.letters().at(0) == "C");
    CHECK(model.tiles().at(0).toMap().value("origin").toString() == "typed");
    CHECK_FALSE(model.complete());

    CHECK(model.setTile(0, "qu"));
    CHECK(model.letters().at(0) == "QU");
    CHECK(model.setTile(0, "abc"));
    CHECK(model.letters().at(0) == "ABC");

    // Not a letter, too many letters, or off the grid: nothing changes.
    CHECK_FALSE(model.setTile(0, "1"));
    CHECK_FALSE(model.setTile(0, "ABCD"));
    CHECK_FALSE(model.setTile(0, ""));
    CHECK_FALSE(model.setTile(-1, "A"));
    CHECK_FALSE(model.setTile(16, "A"));
    CHECK(model.letters().at(0) == "ABC");

    CHECK(model.clearTile(0));
    CHECK(model.letters().at(0).isEmpty());
    CHECK_FALSE(model.acceptTile(0));  // an empty tile cannot be accepted
}

TEST_CASE("a full grid confirms and an incomplete one does not", "[app][scan]") {
    ScanModel model;
    model.typeBoard();
    fill(model, "A");

    CHECK(model.complete());
    CHECK(model.confirm());
    REQUIRE(model.confirmedBoard().has_value());
    CHECK(model.confirmedBoard()->rows() == 4);
    CHECK(model.confirmedBoard()->columns() == 4);
    CHECK(model.confirmedBoard()->tile({0, 0}) == "A");
    CHECK(model.confirmedBoard()->tile({3, 3}) == "A");

    // One tile missing: no board. The confirmed one before is no longer the
    // draft's either.
    CHECK(model.clearTile(15));
    CHECK_FALSE(model.complete());
    CHECK_FALSE(model.confirm());
    CHECK_FALSE(model.confirmedBoard().has_value());

    // The size can be changed, and what overlaps is kept from the top left.
    CHECK(model.resize(3, 3));
    CHECK(model.rows() == 3);
    CHECK(model.columns() == 3);
    CHECK(model.size() == 3);
    CHECK(model.letters().at(0) == "A");
    CHECK(model.letters().size() == 9);

    // A quarter turn keeps the letters and the doubts; only their places move.
    CHECK(model.setTile(0, "Q"));
    model.turn();
    CHECK(model.rows() == 3);
    CHECK(model.letters().at(0) == "A");
    CHECK(model.letters().at(2) == "Q");
}

TEST_CASE("the reader's doubts are the tiles to ring, with its other thoughts", "[app][scan]") {
    ScanModel model;
    CHECK(model.useReading(two_by_two()));

    CHECK(model.state() == "check");
    CHECK(model.rows() == 2);
    CHECK(model.columns() == 2);
    CHECK(letters_of(model) == QStringList({"C", "A", "", "T"}));
    CHECK_FALSE(model.complete());

    // A tile the reader saw nothing in is empty and doubtful; a read tile keeps
    // the reader's doubt.
    CHECK(model.doubtful() == indices({1, 2, 3}));
    CHECK(model.tiles().at(1).toMap().value("origin").toString() == "read");
    CHECK(model.tiles().at(2).toMap().value("origin").toString() == "empty");
    CHECK(model.tiles().at(2).toMap().value("doubtful").toBool());
    CHECK(model.alternatives(1) == QStringList({"O"}));
    CHECK(model.alternatives(0).isEmpty());

    // "This one is right" settles a doubt; an empty tile cannot be accepted.
    CHECK(model.acceptTile(1));
    CHECK(model.doubtful() == indices({2, 3}));
    CHECK_FALSE(model.acceptTile(2));

    // A tap corrects the empty tile; the picker's suggestions come first.
    CHECK(model.setTile(2, "R"));
    CHECK(model.letters().at(2) == "R");
    CHECK(model.doubtful() == indices({3}));

    // "Yes, play" does not refuse a board with a ringed tile left: the press is
    // the grown-up saying they looked (docs/design/scanner.md, "Confirm and
    // correct").
    CHECK(model.complete());
    CHECK(model.confirm());
    REQUIRE(model.confirmedBoard().has_value());
    CHECK(model.confirmedBoard()->tile({1, 0}) == "R");
    CHECK(model.confirmedBoard()->tile({0, 0}) == "C");
}

TEST_CASE("a reading that is not a grid is refused", "[app][scan]") {
    ScanModel model;
    wjs::scan::BoardReading reading;
    reading.rows = 0;
    CHECK_FALSE(model.useReading(reading));
    CHECK(model.state() == "ready");

    reading.rows = 2;
    reading.columns = 2;
    reading.tiles.resize(3);  // not rows * columns
    CHECK_FALSE(model.useReading(reading));
    CHECK(model.state() == "ready");
}

TEST_CASE("without a camera the typed board is the way on, and nothing is faked", "[app][scan]") {
    ScanModel model;
    model.open();
    // The three desktops' camera interface answers `unavailable`
    // (src/platforms/detail/camera_unavailable.cpp): no capture is pretended.
    CHECK_FALSE(model.cameraAvailable());
    CHECK_FALSE(model.cameraCapturing());
    CHECK(model.cameraProblem() == "unavailable");

    model.capture();
    CHECK(model.state() == "ready");
    CHECK(model.readProblem() == "no_camera");

    // Read again from the check screen goes back to ready with the camera on
    // again (or, here, unavailable again), and the draft is gone.
    model.typeBoard();
    CHECK(model.state() == "check");
    model.readAgain();
    CHECK(model.state() == "ready");
    CHECK(model.rows() == 0);
    CHECK_FALSE(model.confirmedBoard().has_value());
    CHECK(model.cameraProblem() == "unavailable");
}
