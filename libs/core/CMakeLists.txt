# The library holds all of the game logic. It is GUI-free and platform-free:
# nothing here includes an operating system header, and what differs between
# operating systems arrives through libs/platforms/interfaces.

# Build-time facts (the version, the target platform's name) become constants
# in a header CMake generates, rather than macros the sources would declare.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sns/version.hpp.in"
    "${PROJECT_BINARY_DIR}/generated/sns/version.hpp"
    @ONLY)

add_library(sns_lib STATIC
    src/board.cpp
    src/cli_arguments.cpp
    src/dashboard.cpp
    src/deal.cpp
    src/dice.cpp
    src/dice_sets.cpp
    src/dictionary.cpp
    src/feedback.cpp
    src/game_rules.cpp
    src/game_session.cpp
    src/grapheme_binder.cpp
    src/language_choice.cpp
    src/lexicon.cpp
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
    src/sound_it_out.cpp
    src/store.cpp
    src/store_format.cpp
    src/version.cpp)
add_library(sns::lib ALIAS sns_lib)

# Which sns/profiling.hpp the zones in the sources resolve to: CMake chooses the
# directory, no preprocessor conditional does (cmake/Tracy.cmake). PUBLIC
# because the layers above the library are instrumented the same way; the
# interface carries the variant's include path and, in a profiling build,
# Tracy's own headers through sns::tracy.
target_include_directories(sns_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}/generated")
target_compile_features(sns_lib PUBLIC cxx_std_23)
target_link_libraries(sns_lib
    PUBLIC
        sns::profiling
    PRIVATE
        sns::warnings
        sns::hardening
        sns::sanitizers
        sns::coverage)
if(SNS_ENABLE_FUZZING)
    add_subdirectory(fuzz)
endif()
