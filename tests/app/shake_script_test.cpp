#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QJSEngine>
#include <QJSValue>
#include <QString>
#include <QStringList>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

#include "dice_sim.hpp"

// The 3D board scene's script (src/app/prototype/shake/qml/ShakeScript.js) in
// Qt's own JavaScript engine, so what is tested is the file the scene plays and
// not a copy, with the dice simulation (DiceSim) it drives. The checks are in
// shake_script_check.js. They are about the dice's faces: that a die lands with
// the face the library's roll says is up, shows other faces on the way, and that
// none of it moves a die before it is sent home. Built only with the shake
// prototype.

namespace {

constexpr std::string_view script_path = WJS_SHAKE_SCRIPT;
constexpr std::string_view check_path = WJS_SHAKE_SCRIPT_CHECK;

QString read_text(std::string_view path) {
    QFile file(QString::fromUtf8(path.data(), static_cast<qsizetype>(path.size())));
    REQUIRE(file.open(QIODevice::ReadOnly));
    return QString::fromUtf8(file.readAll());
}

}  // namespace

TEST_CASE("the shake script lands each die on the face the roll says and tumbles the others",
          "[app][shake]") {
    QString script = read_text(script_path);
    // QML reads this line; a plain script engine does not.
    script.replace(QStringLiteral(".pragma library"), QString());
    const QString check = read_text(check_path);

    const QString program =
        QStringLiteral("(function () {\n") + script + QStringLiteral("\n") + check +
        QStringLiteral(
            "\nvar faults = run({ create: create, advance: advance, evaluate: evaluate, "
            "at: at, digest: digest, finish: finish, faceReport: faceReport, "
            "sides: sides, faceUp: faceUp, topSide: topSide, quatRotate: quatRotate, "
            "sim: diceSim });\n"
            "return { faults: faults, summary: summary };\n})()");

    // The engine needs an application object, and Catch2's main makes none.
    int argc = 1;
    char name[] = "wjs_shake_script_tests";
    char* argv[] = {name, nullptr};
    QCoreApplication application(argc, argv);

    QJSEngine engine;
    DiceSim sim;
    QJSEngine::setObjectOwnership(&sim, QJSEngine::CppOwnership);
    engine.globalObject().setProperty(QStringLiteral("diceSim"), engine.newQObject(&sim));
    const QJSValue result = engine.evaluate(program, QStringLiteral("shake_script_check"));
    INFO("the script or its checks did not run: " << result.toString().toStdString());
    REQUIRE_FALSE(result.isError());

    const QStringList faults = result.property(QStringLiteral("faults")).toVariant().toStringList();
    const QStringList summary =
        result.property(QStringLiteral("summary")).toVariant().toStringList();
    std::string said;
    for (const QString& line : summary) {
        said += line.toStdString() + "\n";
    }
    INFO(said);
    for (const QString& fault : faults) {
        FAIL_CHECK(fault.toStdString());
    }
    CHECK(faults.isEmpty());
    CHECK(summary.size() >= 6);
}
