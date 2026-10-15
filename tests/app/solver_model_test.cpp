#include <QString>
#include <QVariantMap>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

#include "solver_model.hpp"

namespace {

// A dictionary file the model can load, removed by the caller.
std::filesystem::path write_words(const char* name, const char* words) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream out(path);
    out << words;
    return path;
}

QString to_qstring(const std::filesystem::path& path) {
    return QString::fromStdU16String(path.u16string());
}

}  // namespace

TEST_CASE("the model solves a board once a dictionary is loaded", "[app]") {
    const auto path = write_words("wjs_app_model_words.txt", "cat\ncare\ndog\n");

    wjs::app::SolverModel model;
    const bool loaded = model.loadDictionary(to_qstring(path));
    std::filesystem::remove(path);

    REQUIRE(loaded);
    CHECK(model.ready());

    model.solve(QStringLiteral("CAT/ORE/DGS"));

    const auto results = model.results();
    REQUIRE(results.size() == 3);
    // Longest first among equal scores: CARE, then CAT, then DOG.
    CHECK(results.at(0).toMap().value(QStringLiteral("word")).toString() == "CARE");
    CHECK(results.at(1).toMap().value(QStringLiteral("word")).toString() == "CAT");
    CHECK(results.at(2).toMap().value(QStringLiteral("word")).toString() == "DOG");
    CHECK(results.at(0).toMap().value(QStringLiteral("path")).toString() == "r1c1 r1c2 r2c2 r2c3");
    CHECK(model.totalScore() == 3);
}

TEST_CASE("a dictionary that cannot be opened leaves the model not ready", "[app]") {
    wjs::app::SolverModel model;

    CHECK_FALSE(model.loadDictionary(QStringLiteral("this/file/does/not/exist.txt")));
    CHECK_FALSE(model.ready());
    CHECK(model.status().contains(QStringLiteral("this/file/does/not/exist.txt")));
}

TEST_CASE("a board that cannot be read clears the results and says why", "[app]") {
    const auto path = write_words("wjs_app_model_words_b.txt", "cat\n");
    wjs::app::SolverModel model;
    REQUIRE(model.loadDictionary(to_qstring(path)));
    std::filesystem::remove(path);

    model.solve(QStringLiteral("CAT/ORE/DGS"));
    REQUIRE(model.results().size() == 1);

    model.solve(QStringLiteral("AB/C"));

    CHECK(model.results().isEmpty());
    CHECK(model.totalScore() == 0);
    CHECK(model.status().contains(QStringLiteral("cannot be read")));
}
