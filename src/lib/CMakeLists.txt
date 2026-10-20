# The library holds all of the game logic. It is GUI-free and platform-free:
# nothing here includes an operating system header, and what differs between
# operating systems arrives through src/platforms/interfaces.

# Build-time facts (the version, the target platform's name) become constants
# in a header CMake generates, rather than macros the sources would declare.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/wjs/version.hpp.in"
    "${PROJECT_BINARY_DIR}/generated/wjs/version.hpp"
    @ONLY)

add_library(wjs_lib STATIC
    src/board.cpp
    src/dashboard.cpp
    src/deal.cpp
    src/dice.cpp
    src/dice_sets.cpp
    src/dictionary.cpp
    src/feedback.cpp
    src/game_rules.cpp
    src/game_session.cpp
    src/grapheme_binder.cpp
    src/motion.cpp
    src/parent_gate.cpp
    src/path.cpp
    src/phase_data.cpp
    src/scoring.cpp
    src/settings.cpp
    src/shake.cpp
    src/seeding.cpp
    src/session_store.cpp
    src/solver.cpp
    src/store.cpp
    src/store_format.cpp
    src/version.cpp)
add_library(wjs::lib ALIAS wjs_lib)

# Which wjs/profiling.hpp the zones in the sources resolve to: CMake chooses the
# directory, no preprocessor conditional does (cmake/Tracy.cmake). PUBLIC
# because the layers above the library are instrumented the same way; the
# interface carries the variant's include path and, in a profiling build,
# Tracy's own headers through wjs::tracy.
target_include_directories(wjs_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}/generated")
target_compile_features(wjs_lib PUBLIC cxx_std_23)
target_link_libraries(wjs_lib
    PUBLIC
        wjs::profiling
    PRIVATE
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)
