# The scanning library: finds a board in a camera frame and reads each die's
# letter. Portable and free of Qt and of any operating system header; it adds no
# third-party dependency (docs/design/scanner.md, "No image dependency"). It links
# the game library for Board and nothing else, and the game library does not link
# it, so the solver stays free of image code.
add_library(sns_scan STATIC
    src/board_finder.cpp
    src/confirmation.cpp
    src/geometry.cpp
    src/glyph_reader.cpp
    src/image.cpp
    src/reader.cpp
    src/segment.cpp
    src/stroke_font.cpp)
add_library(sns::scan ALIAS sns_scan)

target_include_directories(sns_scan PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(sns_scan PUBLIC cxx_std_23)
target_link_libraries(sns_scan
    PUBLIC
        sns::lib
    PRIVATE
        sns::warnings
        sns::hardening
        sns::sanitizers
        sns::coverage)
if(SNS_ENABLE_FUZZING)
    add_subdirectory(fuzz)
endif()
