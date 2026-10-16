# The dice simulation (docs/design/shake-prototype.md, "The two scripts"): rigid cubes
# in a box, by sequential impulses. Plain C++ with no Qt and no
# renderer, so the tests can run it headless and a different scene could use it.
add_library(wjs_dice_world STATIC
    dice_world.cpp
    dice_world.hpp
    math3.hpp)
target_include_directories(wjs_dice_world PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(wjs_dice_world PUBLIC cxx_std_23)
target_link_libraries(wjs_dice_world
    PRIVATE
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)
