# The scanning library: finds a board in a camera frame and reads each die's
# letter. Portable and free of Qt and of any operating system header; it adds no
# third-party dependency (docs/design/scanner.md, "No image dependency"). It links
# the game library for Board and nothing else, and the game library does not link
# it, so the solver stays free of image code.
add_library(wjs_scan STATIC
    src/board_finder.cpp
    src/confirmation.cpp
    src/geometry.cpp
    src/glyph_reader.cpp
    src/image.cpp
    src/reader.cpp
    src/segment.cpp
    src/stroke_font.cpp)
add_library(wjs::scan ALIAS wjs_scan)

target_include_directories(wjs_scan PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(wjs_scan PUBLIC cxx_std_23)
target_link_libraries(wjs_scan
    PUBLIC
        wjs::lib
    PRIVATE
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)
