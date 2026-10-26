// SPDX-License-Identifier: GPL-3.0-or-later
#include <QCoreApplication>
#include <QFile>
#include <QJSEngine>
#include <QJSValue>
#include <QString>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <set>
#include <string_view>

#include "budgets.hpp"

// The board scene's script (apps/app/qml/solo/SceneScript.js) in Qt's own JavaScript engine, so
// what is tested is the file the scene plays and not a copy. The script is pure functions of the
// board's size, the seed and the time. Toybox is pixel art, so what is held here is that nothing is
// smooth: the box hops in whole steps of 6 px, the lid lifts in quarters, a die falls through six
// frames in whole pixels, and a play has the length the design gives it
// (docs/design/toybox/DESIGN.md, "Motion").

namespace {

constexpr std::string_view script_path = WJS_SCENE_SCRIPT;

QCoreApplication& application() {
    static int argc = 1;
    static char name[] = "wjs_scene_script_tests";
    static char* argv[] = {name, nullptr};
    static QCoreApplication instance(argc, argv);
    return instance;
}

// An engine with the script's functions in its global scope.
class Script {
public:
    Script() {
        QFile file(
            QString::fromUtf8(script_path.data(), static_cast<qsizetype>(script_path.size())));
        REQUIRE(file.open(QIODevice::ReadOnly));
        QString text = QString::fromUtf8(file.readAll());
        // QML reads this line; a plain script engine does not.
        text.replace(QStringLiteral(".pragma library"), QString());
        const QJSValue result = engine_.evaluate(text, QStringLiteral("SceneScript.js"));
        INFO("the script did not run: " << result.toString().toStdString());
        REQUIRE_FALSE(result.isError());
    }

    QJSValue eval(const QString& expression) {
        const QJSValue value = engine_.evaluate(expression);
        INFO(expression.toStdString() << ": " << value.toString().toStdString());
        REQUIRE_FALSE(value.isError());
        return value;
    }
    [[nodiscard]] double number(const QString& expression) { return eval(expression).toNumber(); }
    [[nodiscard]] bool truth(const QString& expression) { return eval(expression).toBool(); }

private:
    // First, so the application exists before the engine does.
    QCoreApplication& application_ = application();
    QJSEngine engine_;
};

}  // namespace

TEST_CASE("a play has the length of its stages", "[scene]") {
    Script s;
    s.eval("var a = create(16, 7, false); var b = create(36, 7, false);");
    CHECK(s.number("stageLength(a, 'shake')") == 600);
    CHECK(s.number("stageLength(a, 'lid')") == 250);
    CHECK(s.number("stageLength(a, 'settle')") == 150);
    // 15 gaps of 40 ms, then the last die's 380 ms of fall.
    CHECK(s.number("stageLength(a, 'drop')") == 15 * 40 + 380);
    // On a 6x6 board the gap shrinks so that all the starts fit in 700 ms.
    CHECK(s.number("stageLength(b, 'drop')") == Catch::Approx(35 * 700.0 / 36 + 380));
}

TEST_CASE("the longest play stays within the budget", "[scene][perf]") {
    Script s;
    s.eval("var b = create(36, 1, false);");
    const double play = s.number(
        "stageLength(b, 'shake') + stageLength(b, 'lid') + "
        "stageLength(b, 'drop') + stageLength(b, 'settle')");
    CHECK(play <= static_cast<double>(wjs::perf::budget::kScenePlay6x6Ms));
}

TEST_CASE("the same seed gives the same play and another seed another", "[scene]") {
    Script s;
    CHECK(s.truth("JSON.stringify(create(16, 5, false)) === JSON.stringify(create(16, 5, false))"));
    CHECK(
        s.truth("JSON.stringify(create(16, 5, false).order) !== JSON.stringify(create(16, 6, "
                "false).order)"));
    // The order is a permutation of the dice.
    CHECK(
        s.truth("create(16, 9, false).order.slice().sort(function (x, y) { return x - y; })"
                ".every(function (v, i) { return v === i; })"));
}

TEST_CASE("the box hops in whole steps of 6 px and never stands still", "[scene]") {
    Script s;
    for (int seed = 1; seed <= 20; ++seed) {
        s.eval(QStringLiteral("var h = create(16, %1, false);").arg(seed));
        CHECK(s.truth("h.hopX.length === 8 && h.hopY.length === 8"));
        CHECK(
            s.truth("h.hopX.concat(h.hopY).every(function (v) { return v === -6 || v === 0 || v "
                    "=== 6; })"));
        CHECK(s.truth("h.hopX.every(function (v, i) { return v !== 0 || h.hopY[i] !== 0; })"));
    }
}

TEST_CASE("the box's position is whole pixels, and only in the shake", "[scene]") {
    Script s;
    s.eval("var a = create(16, 3, false);");
    CHECK(
        s.truth("(function () {"
                "  for (var t = 0; t < 3000; t += 7) {"
                "    var h = boxHop(a, 'shake', t, t % 3 / 2, (t % 5 - 2) / 2);"
                "    if (h.x !== Math.round(h.x) || h.y !== Math.round(h.y)) return false;"
                "  }"
                "  return true; })()"));
    for (const char* stage : {"idle", "lid", "drop", "settle"}) {
        CHECK(s.truth(QStringLiteral("var h = boxHop(a, '%1', 100, 1, 1); h.x === 0 && h.y === 0")
                          .arg(stage)));
    }
    // It holds one hop for a whole step (75 ms), then moves on.
    CHECK(
        s.truth("var p = boxHop(a, 'shake', 5, 0, 0), q = boxHop(a, 'shake', 70, 0, 0);"
                "p.x === q.x && p.y === q.y"));
}

TEST_CASE("the lid lifts in four steps, or cross-fades when motion is reduced", "[scene]") {
    Script s;
    s.eval("var a = create(16, 3, false), r = create(16, 3, true);");
    CHECK(s.number("lidOpen(a, 'shake', 400)") == 0);
    CHECK(s.number("lidOpen(a, 'drop', 0)") == 1);
    CHECK(
        s.truth("(function () {"
                "  var seen = {};"
                "  for (var t = 0; t < 250; t += 5) { seen[lidOpen(a, 'lid', t)] = true; }"
                "  return Object.keys(seen).sort().join() === '0,0.25,0.5,0.75'; })()"));
    CHECK(s.truth("lidOpen(r, 'lid', 30) === 30 / 120"));
}

TEST_CASE("a die falls through six frames, in whole pixels, one after another", "[scene]") {
    Script s;
    s.eval("var a = create(16, 11, false); var tile = 100;");
    // Before its start a die is not seen; then its frames run 0 to 5; then it is down.
    CHECK(s.truth(
        "(function () {"
        "  for (var i = 0; i < 16; ++i) {"
        "    var last = -2, frames = {};"
        "    for (var t = 0; t <= stageLength(a, 'drop'); t += 5) {"
        "      var p = pose(a, 'drop', t, i, tile);"
        "      if (p.frame < last) return 'frames went back for die ' + i;"
        "      last = p.frame; frames[p.frame] = true;"
        "      if (p.x !== Math.round(p.x) || p.y !== Math.round(p.y) || p.y > 0) return 'bad y';"
        "      if ((p.frame < 0) !== (p.opacity === 0)) return 'opacity';"
        "    }"
        "    if (Object.keys(frames).sort().join() !== '-1,0,1,2,3,4,5,6' && i !== a.order[0])"
        "      return 'frames ' + Object.keys(frames).join() + ' for die ' + i;"
        "  }"
        "  return true; })() === true"));
    // The first die falls from 90 percent of a tile up and ends at its place.
    CHECK(s.number("var f = a.order[0]; pose(a, 'drop', 0, f, tile).y") == -90);
    CHECK(s.number("pose(a, 'drop', 379, a.order[0], tile).y") == 0);
    // Every die has landed (frame 3) at its landing time, and the dice start 40 ms apart.
    CHECK(s.number("landingTime(a, a.order[0])") == 190);
    CHECK(s.number("landingTime(a, a.order[1]) - landingTime(a, a.order[0])") == 40);
    CHECK(s.number("frame(a, landingTime(a, 4), 4)") == 3);
    // In the shake and the lid the dice are inside the closed box.
    CHECK(
        s.truth("pose(a, 'shake', 100, 3, tile).opacity === 0 && pose(a, 'lid', 100, 3, "
                "tile).opacity === 0"));
    CHECK(s.truth(
        "var p = pose(a, 'settle', 10, 3, tile); p.x === 0 && p.y === 0 && p.opacity === 1"));
}

TEST_CASE("with reduced motion the dice fade in together and nothing hops", "[scene]") {
    Script s;
    s.eval("var r = create(16, 4, true); var tile = 100;");
    CHECK(s.number("stageLength(r, 'shake')") == 300);
    CHECK(s.number("stageLength(r, 'lid')") == 120);
    CHECK(s.number("stageLength(r, 'drop')") == 120);
    CHECK(s.truth("var h = boxHop(r, 'shake', 100, 1, 1); h.x === 0 && h.y === 0"));
    CHECK(s.number("pose(r, 'drop', 60, 5, tile).opacity") == 0.5);
    CHECK(s.truth("var p = pose(r, 'drop', 60, 5, tile); p.x === 0 && p.y === 0"));
    CHECK(s.number("pose(r, 'shake', 10, 5, tile).opacity") == 0);
    // Every die lands at once.
    CHECK(s.truth("landingTime(r, 0) === landingTime(r, 9)"));
}
