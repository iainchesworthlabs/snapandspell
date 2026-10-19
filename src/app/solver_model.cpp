#include "solver_model.hpp"

#include <QCoreApplication>
#include <QStringList>
#include <QVariantMap>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

#include "wjs/board.hpp"
#include "wjs/platform/paths.hpp"
#include "wjs/profiling.hpp"
#include "wjs/solver.hpp"

namespace wjs::app {
namespace {

// A filesystem path from Qt text, and Qt text from one. Going through UTF-16
// and UTF-8 keeps both portable: the standard converts to the native encoding.
[[nodiscard]] std::filesystem::path to_path(const QString& text) {
    return std::filesystem::path{text.toStdU16String()};
}

[[nodiscard]] QString to_qstring(const std::filesystem::path& path) {
    const auto utf8 = path.u8string();
    return QString::fromUtf8(reinterpret_cast<const char*>(utf8.data()),
                             static_cast<qsizetype>(utf8.size()));
}

[[nodiscard]] QString describe_path(const std::vector<wjs::Position>& path) {
    QStringList steps;
    for (const auto& position : path) {
        steps.append(QStringLiteral("r%1c%2").arg(position.row + 1).arg(position.column + 1));
    }
    return steps.join(QLatin1Char(' '));
}

}  // namespace

SolverModel::SolverModel(QObject* parent) : QObject(parent) {
    setStatus(tr("No dictionary loaded."));
}

void SolverModel::loadDefaultDictionary() {
    // --dictionary <file>, if the program was started with one.
    const QStringList arguments = QCoreApplication::arguments();
    const qsizetype flag = arguments.indexOf(QStringLiteral("--dictionary"));
    if (flag >= 0 && flag + 1 < arguments.size()) {
        loadDictionary(arguments.at(flag + 1));
        return;
    }

    const auto beside_executable =
        wjs::platform::executable_path().parent_path() / "dictionaries" / "en.txt";
    loadDictionary(to_qstring(beside_executable));
}

bool SolverModel::loadDictionary(const QString& path) {
    auto loaded = wjs::Dictionary::load_file(to_path(path));
    if (!loaded) {
        ready_ = false;
        dictionary_ = wjs::Dictionary{};
        setStatus(tr("Could not open the dictionary %1. Start the program with "
                     "--dictionary <file>.")
                      .arg(path));
        return false;
    }

    dictionary_ = std::move(*loaded);
    ready_ = true;
    setStatus(tr("%n word(s) in the dictionary.", nullptr, static_cast<int>(dictionary_.size())));
    return true;
}

void SolverModel::solve(const QString& boardText, int minLength) {
    // The model's half of a solve, over the library's `solver.solve` inside:
    // the parse, the result maps and the signals. The value is the words found.
    auto zone = wjs::profiling::zone<"solver.model">();
    results_.clear();
    total_score_ = 0;

    const auto board = wjs::Board::parse(boardText.toStdString());
    if (!board) {
        emit resultsChanged();
        setStatus(tr("The board cannot be read: %1.")
                      .arg(QString::fromUtf8(wjs::to_string(board.error()).data())));
        return;
    }
    if (!ready_) {
        emit resultsChanged();
        return;
    }

    wjs::SolveOptions options;
    options.min_length = minLength < 1 ? 1U : static_cast<std::size_t>(minLength);
    const auto found = wjs::solve(*board, dictionary_, options);
    zone.Value(static_cast<std::uint64_t>(found.size()));

    for (const auto& word : found) {
        QVariantMap entry;
        entry.insert(QStringLiteral("word"), QString::fromStdString(word.word));
        entry.insert(QStringLiteral("score"), word.score);
        entry.insert(QStringLiteral("path"), describe_path(word.path));
        results_.append(entry);
        total_score_ += word.score;
    }

    emit resultsChanged();
    setStatus(tr("%n word(s) found.", nullptr, static_cast<int>(found.size())));
}

void SolverModel::setStatus(QString status) {
    status_ = std::move(status);
    emit statusChanged();
}

}  // namespace wjs::app
