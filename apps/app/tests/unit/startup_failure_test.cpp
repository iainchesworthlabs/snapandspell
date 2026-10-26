// SPDX-License-Identifier: GPL-3.0-or-later
#include <QByteArray>
#include <QString>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>

#include "sns/platform/fatal_error.hpp"
#include "startup_failure.hpp"

// How the program says that it cannot start. The program has no console on
// Windows, so a missing data file used to be a program that never appeared; the
// message goes where the person can see it, and the exit code says which reason it
// was. The dialog itself needs a desktop and is not shown here: the platform call
// is made with the dialog off, and the app's own decisions are checked with a
// sink in its place.

namespace {

using sns::app::ExitStatus;

struct Told {
    std::string title;
    std::string message;
    bool interactive = false;
    int calls = 0;
};

// Holds SNS_NO_DIALOGS to a value for one case and puts it back.
class ScopedNoDialogs {
public:
    explicit ScopedNoDialogs(bool set) : was_(qgetenv("SNS_NO_DIALOGS")), had_(!was_.isNull()) {
        apply(set);
    }
    ~ScopedNoDialogs() {
        if (had_) {
            qputenv("SNS_NO_DIALOGS", was_);
        } else {
            qunsetenv("SNS_NO_DIALOGS");
        }
    }
    ScopedNoDialogs(const ScopedNoDialogs&) = delete;
    ScopedNoDialogs& operator=(const ScopedNoDialogs&) = delete;

private:
    static void apply(bool set) {
        if (set) {
            qputenv("SNS_NO_DIALOGS", "1");
        } else {
            qunsetenv("SNS_NO_DIALOGS");
        }
    }
    QByteArray was_;
    bool had_;
};

}  // namespace

TEST_CASE("each reason the program can leave for has a code of its own", "[app][startup]") {
    const std::set<int> codes{
        sns::app::exit_code(ExitStatus::ok), sns::app::exit_code(ExitStatus::setup_failed),
        sns::app::exit_code(ExitStatus::no_frame), sns::app::exit_code(ExitStatus::window_failed)};
    CHECK(codes.size() == 4);
    CHECK(sns::app::exit_code(ExitStatus::ok) == 0);
    // 1 is the toolkit's and a crash's, never ours.
    CHECK_FALSE(codes.contains(1));
}

TEST_CASE("a dialog is wanted unless a test or a headless run is in charge", "[app][startup]") {
    CHECK(sns::app::dialogs_wanted("", false));
    CHECK(sns::app::dialogs_wanted("windows", false));
    CHECK_FALSE(sns::app::dialogs_wanted("offscreen", false));
    CHECK_FALSE(sns::app::dialogs_wanted("minimal", false));
    CHECK_FALSE(sns::app::dialogs_wanted("windows", true));
    CHECK_FALSE(sns::app::dialogs_wanted("", true));
}

TEST_CASE("a startup failure is told through the sink once and gives its exit code",
          "[app][startup]") {
    const ScopedNoDialogs env(true);
    Told told;
    const int code = sns::app::fail_startup(
        ExitStatus::setup_failed, QStringLiteral("The word list 'no-such.txt' could not be read."),
        [&told](std::string_view title, std::string_view message, bool interactive) {
            told.title = std::string{title};
            told.message = std::string{message};
            told.interactive = interactive;
            ++told.calls;
        });
    CHECK(code == sns::app::exit_code(ExitStatus::setup_failed));
    CHECK(told.calls == 1);
    CHECK(told.message == "The word list 'no-such.txt' could not be read.");
    CHECK(told.title.find("cannot start") != std::string::npos);
    // A test run must never wait for a click.
    CHECK_FALSE(told.interactive);
}

TEST_CASE("the message keeps its non-ASCII text as UTF-8", "[app][startup]") {
    const ScopedNoDialogs env(true);
    Told told;
    const int code = sns::app::fail_startup(
        ExitStatus::window_failed, QString::fromUtf8("Le fichier \xC3\xA9t\xC3\xA9 perdu."),
        [&told](std::string_view, std::string_view message, bool) {
            told.message = std::string{message};
        });
    CHECK(code == sns::app::exit_code(ExitStatus::window_failed));
    CHECK(told.message == "Le fichier \xC3\xA9t\xC3\xA9 perdu.");
}

TEST_CASE("the platform writes a fatal error to the error stream without a dialog",
          "[app][startup]") {
    std::ostringstream captured;
    std::streambuf* const before = std::cerr.rdbuf(captured.rdbuf());
    sns::platform::show_fatal_error("Snap and Spell cannot start", "no dice", false);
    std::cerr.rdbuf(before);
    CHECK(captured.str() == "Snap and Spell cannot start: no dice\n");
}
