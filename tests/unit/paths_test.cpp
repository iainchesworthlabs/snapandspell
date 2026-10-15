#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>

#include "wjs/platform/paths.hpp"

TEST_CASE("the executable path names the running test program", "[platform]") {
    const auto path = wjs::platform::executable_path();

    REQUIRE_FALSE(path.empty());
    CHECK(path.is_absolute());
    CHECK(std::filesystem::is_regular_file(path));
    CHECK(path.filename().string().find("wjs_unit_tests") != std::string::npos);
}
