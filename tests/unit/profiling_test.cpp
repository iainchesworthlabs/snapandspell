#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <type_traits>

#include "wjs/profiling.hpp"

namespace {

// Whether this build is meant to have Tracy behind the calls: what CMake was
// asked for, passed to this target as a compile definition (tests/CMakeLists.txt).
constexpr bool profiling_expected = WJS_TEST_EXPECT_PROFILING != 0;

// What is the same in both variants: a zone is built in place and never copied
// or moved, as Tracy's own is.
static_assert(!std::is_copy_constructible_v<wjs::profiling::Zone>);
static_assert(!std::is_move_constructible_v<wjs::profiling::Zone>);
static_assert(!std::is_copy_assignable_v<wjs::profiling::Zone>);
static_assert(!std::is_move_assignable_v<wjs::profiling::Zone>);

// A name has to be a literal: Tracy keeps the pointer.
constexpr char persistent_name[] = "test.persistent";
static_assert(std::is_constructible_v<wjs::profiling::Literal, const char (&)[5]>);

}  // namespace

TEST_CASE("the profiling variant CMake chose is the one that was asked for", "[profiling]") {
    CHECK(wjs::profiling::enabled == profiling_expected);
}

TEST_CASE("every profiling call can be made with no profiler connected", "[profiling]") {
    // Nothing observable happens, in either variant: the build either does not
    // have Tracy, or has it in on-demand mode with nobody listening. What this
    // holds is that the calls compile, link and do not fail.
    auto zone = wjs::profiling::zone<"test.zone">();
    zone.Value(7);
    zone.Text("text", 4);

    wjs::profiling::frame_mark();
    wjs::profiling::frame_mark("Test");
    wjs::profiling::plot("test.integer", 3);
    wjs::profiling::plot("test.unsigned", std::uint64_t{3});
    wjs::profiling::plot("test.float", 0.5F);
    wjs::profiling::plot("test.double", 0.5);
    wjs::profiling::plot("test.bool", true);
    wjs::profiling::plot(persistent_name, 1);
    wjs::profiling::message("test.message");
    wjs::profiling::thread_name("test.thread");

    SUCCEED();
}

TEST_CASE("one zone name used twice is one source location", "[profiling]") {
    // A zone's location is a function-local static of the template, so a name
    // used from two places is built once; it must still run from both.
    for (int pass = 0; pass < 2; ++pass) {
        auto zone = wjs::profiling::zone<"test.twice">();
        zone.Value(static_cast<std::uint64_t>(pass));
    }
    SUCCEED();
}
