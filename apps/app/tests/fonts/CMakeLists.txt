# The house faces for the test programs that start Qt Quick (test_fonts.hpp). A static library of
# its own so that the Quick Test harness and both stub shells share one list; the files are read from
# assets/fonts, the directory the application's resource is made from.
add_library(sns_test_fonts STATIC test_fonts.cpp test_fonts.hpp)
target_include_directories(sns_test_fonts PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(sns_test_fonts PRIVATE
    "SNS_TEST_FONTS_DIRECTORY=\"${PROJECT_SOURCE_DIR}/assets/fonts\"")
target_link_libraries(sns_test_fonts
    PUBLIC
        Qt6::Gui
    PRIVATE
        sns::warnings
        sns::sanitizers
        sns::coverage)
