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
    src/dictionary.cpp
    src/path.cpp
    src/phase_data.cpp
    src/scoring.cpp
    src/solver.cpp)
add_library(wjs::lib ALIAS wjs_lib)

target_include_directories(wjs_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}/generated")
target_compile_features(wjs_lib PUBLIC cxx_std_23)
target_link_libraries(wjs_lib
    PRIVATE
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)
