# The shake prototype's dice simulation: plain C++, tested without Qt or a window.
if(TARGET wjs_dice_world)
    add_executable(wjs_dice_tests dice_faces_test.cpp dice_world_test.cpp)
    target_link_libraries(wjs_dice_tests
        PRIVATE
            wjs_dice_world
            wjs::warnings
            wjs::sanitizers
            wjs::coverage
            Catch2::Catch2WithMain)
    wjs_deploy_sanitizer_runtime(wjs_dice_tests)
    catch_discover_tests(wjs_dice_tests
        TEST_PREFIX "unit."
        PROPERTIES LABELS "unit")
endif()
