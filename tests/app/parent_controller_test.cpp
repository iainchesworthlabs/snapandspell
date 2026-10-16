#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#include "demo_data.hpp"
#include "parent_controller.hpp"
#include "wjs/calendar.hpp"
#include "wjs/store.hpp"

namespace {

using wjs::app::ParentController;

constexpr wjs::Day friday = 20742;  // 2026-10-16

// A directory of its own that is gone when the case ends.
class TempDirectory {
public:
    TempDirectory() {
        static int counter = 0;
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("wjs_parent_test_" + std::to_string(stamp) + "_" + std::to_string(++counter));
        std::filesystem::create_directories(path_);
    }
    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;
    ~TempDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }
    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

// A controller on the made-up play of the demo data, as of a Friday.
struct Fixture {
    wjs::PhaseSet phases = wjs::app::demo::phases();
    wjs::Store store = wjs::app::demo::busy_store(friday);
    ParentController controller;

    Fixture() {
        controller.useCalendar([] { return friday; }, 1);
        controller.useStore(&store);
        controller.usePhases(&phases);
    }
};

void open_gate(ParentController& controller) {
    controller.layoutGate(1000.0, 700.0, 48.0);
    const QVariantList spots = controller.gateSpots();
    REQUIRE(spots.size() == 2);
    controller.gateTouchDown(1, spots.at(0).toMap().value("x").toDouble(),
                             spots.at(0).toMap().value("y").toDouble(), 0);
    controller.gateTouchDown(2, spots.at(1).toMap().value("x").toDouble(),
                             spots.at(1).toMap().value("y").toDouble(), 0);
    controller.gateTick(2000);
}

QVariantMap map(const QVariant& value) {
    return value.toMap();
}

}  // namespace

// ---------------------------------------------------------------------------
// The gate

TEST_CASE("a new controller is locked and holds nothing", "[app][parent]") {
    ParentController controller;

    CHECK_FALSE(controller.unlocked());
    CHECK_FALSE(controller.gateHolding());
    CHECK(controller.gateProgress() == 0.0);
    CHECK(map(controller.dashboard()).value("empty").toBool());
}

TEST_CASE("the gate lays two spots out for the screen it is given", "[app][parent][gate]") {
    ParentController controller;

    controller.layoutGate(1000.0, 700.0, 48.0);

    const QVariantList spots = controller.gateSpots();
    REQUIRE(spots.size() == 2);
    CHECK(map(spots.at(0)).value("x").toDouble() == 250.0);
    CHECK(map(spots.at(1)).value("x").toDouble() == 750.0);
    CHECK(map(spots.at(0)).value("r").toDouble() >= 48.0);
}

TEST_CASE("two fingers held for two seconds open the gate", "[app][parent][gate]") {
    ParentController controller;
    int opened = 0;
    QObject::connect(&controller, &ParentController::unlockedChanged, [&opened] { ++opened; });
    controller.layoutGate(1000.0, 700.0, 48.0);
    const QVariantList spots = controller.gateSpots();
    const auto at = [&spots](int i, const char* axis) {
        return spots.at(i).toMap().value(axis).toDouble();
    };

    controller.gateTouchDown(1, at(0, "x"), at(0, "y"), 100);
    CHECK_FALSE(controller.gateHolding());
    controller.gateTouchDown(2, at(1, "x"), at(1, "y"), 100);
    CHECK(controller.gateHolding());

    controller.gateTick(1100);
    CHECK(controller.gateProgress() == 0.5);
    CHECK_FALSE(controller.unlocked());
    controller.gateTick(2099);
    CHECK_FALSE(controller.unlocked());
    controller.gateTick(2100);
    CHECK(controller.unlocked());
    CHECK(controller.gateProgress() == 1.0);
    CHECK(opened == 1);

    controller.gateTick(9999);
    CHECK(opened == 1);  // told once
}

TEST_CASE("lifting a finger early starts the hold over", "[app][parent][gate]") {
    ParentController controller;
    controller.layoutGate(1000.0, 700.0, 48.0);
    const QVariantList spots = controller.gateSpots();
    const double x0 = spots.at(0).toMap().value("x").toDouble();
    const double y0 = spots.at(0).toMap().value("y").toDouble();
    const double x1 = spots.at(1).toMap().value("x").toDouble();
    const double y1 = spots.at(1).toMap().value("y").toDouble();

    controller.gateTouchDown(1, x0, y0, 0);
    controller.gateTouchDown(2, x1, y1, 0);
    controller.gateTouchUp(2, 1500);
    controller.gateTick(5000);

    CHECK_FALSE(controller.unlocked());
    CHECK_FALSE(controller.gateHolding());
    CHECK(controller.gateProgress() == 0.0);
}

TEST_CASE("the accessibility action opens the gate with no hold", "[app][parent][gate]") {
    ParentController controller;

    controller.unlockForAssistiveTechnology();

    CHECK(controller.unlocked());
}

TEST_CASE("leaving shuts the gate again", "[app][parent][gate]") {
    ParentController controller;
    open_gate(controller);
    REQUIRE(controller.unlocked());

    controller.lock();

    CHECK_FALSE(controller.unlocked());
    CHECK(controller.gateProgress() == 0.0);
}

TEST_CASE("opening the gate computes the dashboard as of that moment", "[app][parent][gate]") {
    Fixture f;
    f.controller.lock();
    const QVariantMap before = map(f.controller.dashboard());

    // Something happens behind the screen's back.
    wjs::BoardSnapshot board;
    board.size = 3;
    board.tiles = "Q Q Q/P I N/T A P";
    board.viable_total = 5;
    board.finds = {{"quiz", false, friday, 5}};
    f.store.record(board, friday);
    CHECK(map(f.controller.dashboard()).value("week").toMap() == before.value("week").toMap());

    open_gate(f.controller);

    CHECK(map(f.controller.dashboard()).value("week").toMap().value("stars").toInt() ==
          before.value("week").toMap().value("stars").toInt() + 5);
}

// ---------------------------------------------------------------------------
// The dashboard

TEST_CASE("the dashboard map has what the dashboard has", "[app][parent][dashboard]") {
    Fixture f;

    const QVariantMap d = map(f.controller.dashboard());

    CHECK_FALSE(d.value("empty").toBool());
    CHECK(d.value("hasPhaseData").toBool());
    CHECK(d.value("phase").toString() == "B");
    CHECK(d.value("sounds").toList().size() == 10);
    CHECK_FALSE(d.value("vault").toList().isEmpty());
    CHECK_FALSE(d.value("best").toList().isEmpty());
    const QVariantMap week = d.value("week").toMap();
    CHECK(week.value("played").toBool());
    CHECK(week.value("boardsCleared").toInt() == 1);
    CHECK(week.value("stars").toInt() > 0);
    CHECK_FALSE(week.value("longestWord").toString().isEmpty());
}

TEST_CASE("a sound's row has its counts", "[app][parent][dashboard]") {
    Fixture f;

    QVariantMap ai;
    for (const QVariant& row : map(f.controller.dashboard()).value("sounds").toList()) {
        if (row.toMap().value("letters").toString() == "ai") {
            ai = row.toMap();
        }
    }

    REQUIRE_FALSE(ai.isEmpty());
    CHECK(ai.value("boardsMet").toInt() == 5);
    CHECK(ai.value("boardsFound").toInt() == 3);
    CHECK(ai.value("wordsFound").toInt() ==
          3);  // pain, stain and paint, each once whatever its finds
    CHECK(ai.value("focus").toBool());
    CHECK_FALSE(ai.value("fewWords").toBool());
}

TEST_CASE("the week follows the calendar the controller was given", "[app][parent][dashboard]") {
    Fixture f;
    const int monday_first =
        map(f.controller.dashboard()).value("week").toMap().value("stars").toInt();

    // Sunday-first: the week of the 16th starts on the 11th, which takes in no more of the demo
    // play.
    f.controller.useCalendar([] { return friday; }, 7);

    CHECK(map(f.controller.dashboard()).value("week").toMap().value("stars").toInt() ==
          monday_first);
    // A calendar that has moved on a week leaves this week without play.
    f.controller.useCalendar([] { return friday + 14; }, 1);
    CHECK_FALSE(map(f.controller.dashboard()).value("week").toMap().value("played").toBool());
}

TEST_CASE("without phase data there are no sounds", "[app][parent][dashboard]") {
    Fixture f;

    f.controller.usePhases(nullptr);

    const QVariantMap d = map(f.controller.dashboard());
    CHECK_FALSE(d.value("hasPhaseData").toBool());
    CHECK(d.value("sounds").toList().isEmpty());
    CHECK(f.controller.focusChoices().isEmpty());
    CHECK_FALSE(f.controller.phonicsAvailable());
}

// ---------------------------------------------------------------------------
// The settings

TEST_CASE("the settings map has the table's defaults", "[app][parent][settings]") {
    ParentController controller;

    const QVariantMap s = controller.settings();

    CHECK(s.value("help").toBool());
    CHECK(s.value("board").toString() == "standard");
    CHECK(s.value("phase").toString() == "A");
    CHECK(s.value("boardSize").toInt() == 3);
    CHECK(s.value("soundItOut").toString() == "full");
    CHECK(s.value("celebrationSounds").toString() == "full");
    CHECK(s.value("sound").toBool());
    CHECK(s.value("vibration").toBool());
    CHECK(s.value("shake").toBool());
    CHECK(s.value("motion").toString() == "system");
    CHECK(s.value("textSizePercent").toInt() == 100);
    CHECK(s.value("schoolFont").toString().isEmpty());
    CHECK(s.value("focusSounds").toStringList().isEmpty());
}

TEST_CASE("each setting can be changed by its name and is kept in the store",
          "[app][parent][settings]") {
    Fixture f;

    CHECK(f.controller.setSetting("help", false));
    CHECK(f.controller.setSetting("board", "standard"));
    CHECK(f.controller.setSetting("phase", "A"));
    CHECK(f.controller.setSetting("boardSize", 4));
    CHECK(f.controller.setSetting("soundItOut", "word"));
    CHECK(f.controller.setSetting("celebrationSounds", "quiet"));
    CHECK(f.controller.setSetting("sound", false));
    CHECK(f.controller.setSetting("vibration", false));
    CHECK(f.controller.setSetting("shake", false));
    CHECK(f.controller.setSetting("motion", "reduced"));
    CHECK(f.controller.setSetting("textSizePercent", 150));

    const wjs::Settings& s = f.store.settings();
    CHECK_FALSE(s.help);
    CHECK(s.phase == "A");
    CHECK(s.board_size == 4);
    CHECK(s.sound_it_out == wjs::SoundItOut::word_only);
    CHECK(s.celebration_sounds == wjs::CelebrationSounds::quiet);
    CHECK_FALSE(s.sound);
    CHECK_FALSE(s.vibration);
    CHECK_FALSE(s.shake);
    CHECK(s.motion == wjs::MotionSetting::reduced);
    CHECK(s.text_size_percent == 150);
    // And the map the screens read is the new one.
    CHECK(f.controller.settings().value("boardSize").toInt() == 4);
}

TEST_CASE("numbers from QML arrive as doubles and are accepted when whole",
          "[app][parent][settings]") {
    ParentController controller;

    CHECK(controller.setSetting("boardSize", 4.0));
    CHECK(controller.settings().value("boardSize").toInt() == 4);
    CHECK(controller.setSetting("textSizePercent", 125.0));
    CHECK_FALSE(controller.setSetting("boardSize", 3.5));
}

TEST_CASE("a value the table does not have is refused and nothing changes",
          "[app][parent][settings]") {
    Fixture f;
    const QVariantMap before = f.controller.settings();

    CHECK_FALSE(f.controller.setSetting("nonsense", true));
    CHECK_FALSE(f.controller.setSetting("help", "yes"));
    CHECK_FALSE(f.controller.setSetting("help", 1));
    CHECK_FALSE(f.controller.setSetting("board", "cubist"));
    CHECK_FALSE(f.controller.setSetting("phase", "Z"));
    CHECK_FALSE(f.controller.setSetting("boardSize", 5));
    CHECK_FALSE(f.controller.setSetting("boardSize", "3"));
    CHECK_FALSE(f.controller.setSetting("soundItOut", "loud"));
    CHECK_FALSE(f.controller.setSetting("celebrationSounds", ""));
    CHECK_FALSE(f.controller.setSetting("motion", true));
    CHECK_FALSE(f.controller.setSetting("textSizePercent", 130));
    CHECK_FALSE(f.controller.setSetting("schoolFont", "Some Font"));

    CHECK(f.controller.settings() == before);
}

TEST_CASE("no school font is offered, because none can be bundled", "[app][parent][settings]") {
    ParentController controller;

    CHECK(controller.schoolFonts().isEmpty());
    CHECK(controller.setSetting("schoolFont", ""));
}

TEST_CASE("the phonics board cannot be chosen without its words", "[app][parent][settings]") {
    ParentController controller;
    CHECK_FALSE(controller.phonicsAvailable());
    CHECK_FALSE(controller.setSetting("board", "phonics"));

    wjs::PhaseSet phases = wjs::app::demo::phases();
    controller.usePhases(&phases);

    CHECK(controller.phonicsAvailable());
    CHECK(controller.setSetting("board", "phonics"));
    CHECK(controller.settings().value("board").toString() == "phonics");
}

TEST_CASE("the phases on offer are the data's, or two generic names", "[app][parent][settings]") {
    ParentController controller;
    CHECK(controller.phases() == QStringList{"A", "B"});

    wjs::PhaseSet phases = wjs::app::demo::phases();
    controller.usePhases(&phases);

    CHECK(controller.phases() == QStringList{"A", "B"});
}

TEST_CASE("only sounds of the phase can be ticked", "[app][parent][settings]") {
    Fixture f;

    CHECK(f.controller.toggleFocusSound("s"));
    CHECK(f.controller.settings().value("focusSounds").toStringList().contains("s"));
    CHECK_FALSE(f.controller.toggleFocusSound("ee"));
    CHECK_FALSE(f.controller.toggleFocusSound(""));
    CHECK_FALSE(f.controller.settings().value("focusSounds").toStringList().contains("ee"));

    // Ticking again unticks.
    CHECK(f.controller.toggleFocusSound("s"));
    CHECK_FALSE(f.controller.settings().value("focusSounds").toStringList().contains("s"));
}

TEST_CASE("changing the phase drops the ticked sounds the new phase does not have",
          "[app][parent][settings]") {
    Fixture f;
    REQUIRE(f.controller.settings().value("focusSounds").toStringList().contains("ai"));

    CHECK(f.controller.setSetting("phase", "A"));

    // "ai" is phase B's.
    CHECK_FALSE(f.controller.settings().value("focusSounds").toStringList().contains("ai"));
    CHECK(f.controller.settings().value("focusSounds").toStringList().contains("z"));
}

TEST_CASE("the screens read the text size and the motion from the settings",
          "[app][parent][settings]") {
    ParentController controller;
    CHECK(controller.textScale() == 1.0);
    CHECK_FALSE(controller.reducedMotion());

    controller.setSetting("textSizePercent", 125);
    controller.setSetting("motion", "reduced");

    CHECK(controller.textScale() == 1.25);
    CHECK(controller.reducedMotion());
}

// ---------------------------------------------------------------------------
// The two resets

TEST_CASE("the first reset clears boards and finds and the second clears stars and stickers",
          "[app][parent][reset]") {
    Fixture f;
    REQUIRE_FALSE(f.store.data().boards.empty());

    f.controller.resetBoardsAndFinds();
    CHECK(f.store.data().boards.empty());
    CHECK(f.store.data().finds.empty());
    CHECK(f.store.data().stars > 0);
    CHECK_FALSE(map(f.controller.dashboard()).value("vault").toList().isEmpty());
    // The days' tallies went with the boards and finds they were made of.
    CHECK(map(f.controller.dashboard()).value("week").toMap().value("boardsCleared").toInt() == 0);
    CHECK(map(f.controller.dashboard()).value("week").toMap().value("stars").toInt() == 0);

    f.controller.resetStarsAndStickers();
    CHECK(f.store.data().stars == 0);
    CHECK(map(f.controller.dashboard()).value("vault").toList().isEmpty());
    CHECK(map(f.controller.dashboard()).value("empty").toBool());
}

TEST_CASE("a reset keeps the settings", "[app][parent][reset]") {
    Fixture f;
    f.controller.setSetting("vibration", false);

    f.controller.resetBoardsAndFinds();
    f.controller.resetStarsAndStickers();

    CHECK_FALSE(f.store.settings().vibration);
}

// ---------------------------------------------------------------------------
// The store, and what the parent is told of it

TEST_CASE("a store in memory is said not to be saved", "[app][parent][store]") {
    ParentController controller;

    CHECK(controller.storeNote() == "unsaved");
}

TEST_CASE("a store in a file is kept, and what is set is there next time", "[app][parent][store]") {
    const TempDirectory dir;
    const auto file = dir.path() / "store.wjs";
    {
        ParentController controller;
        controller.openStore(file);
        CHECK(controller.storeNote().isEmpty());
        CHECK(controller.setSetting("boardSize", 4));
    }

    ParentController again;
    again.openStore(file);

    CHECK(again.settings().value("boardSize").toInt() == 4);
    CHECK(again.storeNote().isEmpty());
}

TEST_CASE("a store that could not be read says so, and the old file is kept",
          "[app][parent][store]") {
    const TempDirectory dir;
    const auto file = dir.path() / "store.wjs";
    {
        std::ofstream out(file, std::ios::binary);
        out << "not a store";
    }

    ParentController controller;
    controller.openStore(file);

    CHECK(controller.storeNote() == "unreadable");
    CHECK(std::filesystem::exists(dir.path() / "store.wjs.unreadable"));
}

TEST_CASE("a store from a newer version says so, and is left alone", "[app][parent][store]") {
    const TempDirectory dir;
    const auto file = dir.path() / "store.wjs";
    const std::string later = "wjs-store 9\nsomething new\nend 0000000000000000\n";
    {
        std::ofstream out(file, std::ios::binary);
        out << later;
    }

    ParentController controller;
    controller.openStore(file);
    controller.setSetting("help", false);

    CHECK(controller.storeNote() == "newer");
    std::ifstream in(file, std::ios::binary);
    const std::string now((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(now == later);
}

TEST_CASE("the version is the project's", "[app][parent]") {
    ParentController controller;

    CHECK_FALSE(controller.version().isEmpty());
}
