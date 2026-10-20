#include <QByteArray>
#include <QCoreApplication>
#include <QFileInfo>
#include <QHash>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <catch2/catch_test_macros.hpp>
#include <string>

#include "budgets.hpp"

// The window's frame budget, measured on the application itself: its smoke run
// (`WordJewelsSolver --smoke <frames>`, docs/profiling.md, "The smoke mode") in
// a child process, and the report it prints. The budget is the design's
// (docs/design/performance.md, "Budgets"): one 60 Hz frame, and no frame over
// 33 ms during the shake and the drop.
//
// It is a wall-time check, so it is treated as the performance tier's are
// (docs/testing.md, "Performance tests"): it skips in a build where a clock is
// not about the program (Debug, coverage, Tracy), and it is opt-in with its own
// variable -- WJS_PERF_FRAME_TIMING=1 -- because it also needs a display and
// must never fail a shared runner. CTest runs it alone (RUN_SERIAL).

namespace {

constexpr bool optimised = WJS_TEST_OPTIMISED != 0;
constexpr bool instrumented = WJS_TEST_INSTRUMENTED != 0;

// The frames measured, after the smoke run's own warm-up. Four seconds at
// 60 Hz: enough shakes and drops for a tail to mean something.
constexpr int kFrames = 240;
constexpr int kTimeoutMs = 180'000;

[[nodiscard]] bool frame_timing_requested() {
    const QByteArray value = qgetenv("WJS_PERF_FRAME_TIMING");
    return !value.isEmpty() && value != "0";
}

// Stands the case aside in a build where a frame interval is not a statement
// about the program, and where nobody asked for one. A call, not `if constexpr`:
// SKIP ends the case, and a compiler that can see it always does would warn
// about the statements after it.
void skip_unless_frame_wanted() {
    if (!optimised) {
        SKIP("a frame interval is not about the program in a build that is not optimised");
    }
    if (instrumented) {
        SKIP(
            "the profiling calls and the coverage counters are compiled in: the frame times of "
            "this build are not the program's");
    }
    if (!frame_timing_requested()) {
        SKIP(
            "the window's frame check is opt-in and needs a display: set "
            "WJS_PERF_FRAME_TIMING=1 on a machine with one (docs/testing.md, \"Performance "
            "tests\")");
    }
    if (qEnvironmentVariable("QT_QPA_PLATFORM")
            .startsWith(QStringLiteral("offscreen"), Qt::CaseInsensitive)) {
        SKIP(
            "Quick 3D does not render on the offscreen platform plugin (docs/profiling.md, "
            "\"The smoke mode\"): its frames are the 2D screens and not the scene");
    }
}

// The application's own binary, beside this test's (the build puts every program
// in one directory).
[[nodiscard]] QString application_path() {
    const QString directory = QFileInfo(QString::fromUtf8(WJS_APP_PATH)).absolutePath();
    for (const QString& name :
         {QStringLiteral("WordJewelsSolver.exe"), QStringLiteral("WordJewelsSolver")}) {
        const QString path = directory + QLatin1Char('/') + name;
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return {};
}

// The one line the smoke run prints for a machine (`smoke: key=value ...`), as
// the numbers it holds.
[[nodiscard]] QHash<QString, double> smoke_summary(const QString& output) {
    QHash<QString, double> values;
    for (const QString& line : output.split(QLatin1Char('\n'))) {
        if (!line.startsWith(QStringLiteral("smoke: "))) {
            continue;
        }
        const QStringList tokens = line.mid(7).split(QLatin1Char(' '), Qt::SkipEmptyParts);
        for (const QString& token : tokens) {
            const qsizetype at = token.indexOf(QLatin1Char('='));
            if (at <= 0) {
                continue;
            }
            bool ok = false;
            const double value = token.mid(at + 1).toDouble(&ok);
            if (ok) {
                values.insert(token.left(at), value);
            }
        }
    }
    return values;
}

// The application object QProcess needs, made once (Catch2's main makes none).
QCoreApplication& application() {
    static int argc = 1;
    static char name[] = "wjs_frame_timing_tests";
    static char* argv[] = {name, nullptr};
    static QCoreApplication instance(argc, argv);
    return instance;
}

}  // namespace

TEST_CASE("the window holds the frame budget through the smoke run's shakes", "[app][frame]") {
    skip_unless_frame_wanted();
    application();

    const QString app = application_path();
    INFO("the application, from WJS_APP_PATH: " << WJS_APP_PATH);
    if (app.isEmpty()) {
        SKIP("the application is not built (WJS_BUILD_APP=OFF, or this test was built alone)");
    }

    QTemporaryDir directory;
    REQUIRE(directory.isValid());

    QProcess process;
    process.setProgram(app);
    process.setArguments({QStringLiteral("--smoke"), QString::number(kFrames),
                          // A store of its own: the smoke writes the boards it
                          // plays, and a run must not touch the person's.
                          QStringLiteral("--store"),
                          directory.filePath(QStringLiteral("store.wjs"))});
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.setWorkingDirectory(QFileInfo(app).absolutePath());
    process.start();
    REQUIRE(process.waitForStarted(kTimeoutMs));
    if (!process.waitForFinished(kTimeoutMs)) {
        process.kill();
        process.waitForFinished(5000);
        FAIL("the smoke run did not finish in " << kTimeoutMs / 1000 << " s");
    }

    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    const QString errors = QString::fromUtf8(process.readAllStandardError());
    INFO("stdout:\n" << output.toStdString());
    INFO("stderr:\n" << errors.toStdString());

    if (process.exitStatus() != QProcess::NormalExit) {
        FAIL("the smoke run was killed or crashed");
    }
    if (process.exitCode() == 3) {
        // The application says no frame was presented: Quick 3D needs a display,
        // and there is not one here (docs/profiling.md, "The smoke mode").
        SKIP("the window could not present a frame: " << errors.toStdString());
    }
    REQUIRE(process.exitCode() == 0);

    const QHash<QString, double> report = smoke_summary(output);
    REQUIRE(report.contains(QStringLiteral("frames")));
    REQUIRE(report.contains(QStringLiteral("p50_ms")));
    REQUIRE(report.contains(QStringLiteral("max_ms")));
    REQUIRE(report.contains(QStringLiteral("over33")));

    INFO("frames " << report.value(QStringLiteral("frames")) << ", p50 "
                   << report.value(QStringLiteral("p50_ms")) << " ms, p95 "
                   << report.value(QStringLiteral("p95_ms")) << " ms, max "
                   << report.value(QStringLiteral("max_ms")) << " ms, over 33 ms "
                   << report.value(QStringLiteral("over33")));

    // The frames asked for, and the warm-up left out of them.
    CHECK(report.value(QStringLiteral("frames")) == static_cast<double>(kFrames));
    // The typical frame: 16.7 ms at 60 Hz, with the budget's own small room.
    CHECK(report.value(QStringLiteral("p50_ms")) <= wjs::perf::budget::kFrameP50Ms);
    // No frame over 33 ms during the shake and the drop.
    CHECK(report.value(QStringLiteral("max_ms")) <= wjs::perf::budget::kFrameMaxMs);
    CHECK(report.value(QStringLiteral("over33")) == 0.0);
    // The report is of a window that drew.
    CHECK(report.value(QStringLiteral("first_frame_ms")) > 0.0);
}
