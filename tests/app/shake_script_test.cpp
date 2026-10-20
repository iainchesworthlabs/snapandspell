#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QJSEngine>
#include <QJSValue>
#include <QString>
#include <QStringList>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>
#include <string_view>

#include "budgets.hpp"
#include "dice_sim.hpp"

// The board scene's script (src/app/qml/scene/ShakeScript.js) in Qt's own
// JavaScript engine, so what is tested is the file the scene plays and not a copy,
// with the dice simulation (DiceSim) it drives. The checks are in
// shake_script_check.js. They are about the dice's faces: that a die lands with
// the face the library's roll says is up, shows other faces on the way, and that
// none of it moves a die before it is sent home.
//
// The same run also counts the scene's work for the performance tier's budgets:
// the simulation steps of one 4x4 play and the cells of a board's atlas. They are
// counts, not times, and are the same on every machine and compiler, as the
// solver's visits are (tests/perf, docs/design/performance.md).

namespace {

constexpr std::string_view script_path = WJS_SHAKE_SCRIPT;
constexpr std::string_view check_path = WJS_SHAKE_SCRIPT_CHECK;

QString read_text(std::string_view path) {
    QFile file(QString::fromUtf8(path.data(), static_cast<qsizetype>(path.size())));
    REQUIRE(file.open(QIODevice::ReadOnly));
    return QString::fromUtf8(file.readAll());
}

// The application object QML's JavaScript engine needs, made once: a test binary
// has one, and Catch2's main makes none.
QCoreApplication& application() {
    static int argc = 1;
    static char name[] = "wjs_shake_script_tests";
    static char* argv[] = {name, nullptr};
    static QCoreApplication instance(argc, argv);
    return instance;
}

// What one run of the script's own checks found.
struct SceneCheck {
    QStringList faults;
    QStringList summary;
    // The work the budgets hold (shake_script_check.js, "work").
    int frames = 0;
    int steps = 0;
    int atlas_cells = 0;
    int atlas_letters = 0;
    int atlas_jewels = 0;
};

[[nodiscard]] SceneCheck run_check() {
    application();

    QString script = read_text(script_path);
    // QML reads this line; a plain script engine does not.
    script.replace(QStringLiteral(".pragma library"), QString());
    const QString check = read_text(check_path);

    const QString program =
        QStringLiteral("(function () {\n") + script + QStringLiteral("\n") + check +
        QStringLiteral(
            "\nreturn run({ create: create, advance: advance, evaluate: evaluate, "
            "at: at, digest: digest, finish: finish, faceReport: faceReport, "
            "sides: sides, faceUp: faceUp, topSide: topSide, quatRotate: quatRotate, "
            "newTracker: newTracker, atlasCells: atlasCells, sim: diceSim });\n"
            "})()");

    QJSEngine engine;
    DiceSim sim;
    QJSEngine::setObjectOwnership(&sim, QJSEngine::CppOwnership);
    engine.globalObject().setProperty(QStringLiteral("diceSim"), engine.newQObject(&sim));
    const QJSValue result = engine.evaluate(program, QStringLiteral("shake_script_check"));
    INFO("the script or its checks did not run: " << result.toString().toStdString());
    REQUIRE_FALSE(result.isError());

    SceneCheck found;
    found.faults = result.property(QStringLiteral("faults")).toVariant().toStringList();
    found.summary = result.property(QStringLiteral("summary")).toVariant().toStringList();
    const QJSValue work = result.property(QStringLiteral("work"));
    found.frames = work.property(QStringLiteral("frames")).toInt();
    found.steps = work.property(QStringLiteral("steps")).toInt();
    found.atlas_cells = work.property(QStringLiteral("atlas")).toInt();
    found.atlas_letters = work.property(QStringLiteral("atlasLetters")).toInt();
    found.atlas_jewels = work.property(QStringLiteral("atlasJewels")).toInt();
    return found;
}

// One run per binary, shared by the cases: the program is a dozen plays, and
// the second case adds no work of its own.
[[nodiscard]] const SceneCheck& scene_check() {
    static const SceneCheck found = run_check();
    return found;
}

void report(const SceneCheck& found) {
    std::string said;
    for (const QString& line : found.summary) {
        said += line.toStdString() + "\n";
    }
    INFO(said);
    for (const QString& fault : found.faults) {
        FAIL_CHECK(fault.toStdString());
    }
    CHECK(found.faults.isEmpty());
}

}  // namespace

TEST_CASE("the shake script lands each die on the face the roll says and tumbles the others",
          "[app][shake]") {
    const SceneCheck& found = scene_check();
    report(found);
    CHECK(found.summary.size() >= 6);
}

TEST_CASE("the scene's work is within its budgets: a play's simulation steps and an atlas's cells",
          "[app][scene][perf]") {
    const SceneCheck& found = scene_check();

    // One 4x4 play at 60 Hz with the stand-in's shake, from the first frame to
    // the last die home. The budget sits a quarter above what it measures; it is
    // the simulation's own count of whole steps, so a change to the step size,
    // the stages or the physics that costs more is a failure with a number.
    INFO("simulation steps of one 4x4 play: measured " << found.steps << ", budget "
                                                       << wjs::perf::budget::kSceneSteps4x4);
    CHECK(found.frames > 0);
    CHECK(found.frames < 1000);  // the play finished rather than hitting the cap
    CHECK(found.steps > 0);
    CHECK(static_cast<std::uint64_t>(found.steps) <= wjs::perf::budget::kSceneSteps4x4);

    // The cells of a board's atlas, which is the glyph and jewel work a board's
    // prepare does: six a die, exact, and the two kinds add up.
    INFO("atlas cells: measured " << found.atlas_cells << " (" << found.atlas_letters
                                  << " letters, " << found.atlas_jewels << " jewels)");
    CHECK(static_cast<std::uint64_t>(found.atlas_cells) == wjs::perf::budget::kSceneAtlasCells);
    CHECK(static_cast<std::uint64_t>(found.atlas_letters) == wjs::perf::budget::kSceneAtlasLetters);
    CHECK(static_cast<std::uint64_t>(found.atlas_jewels) == wjs::perf::budget::kSceneAtlasJewels);
}
