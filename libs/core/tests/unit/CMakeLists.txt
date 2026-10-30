add_executable(sns_unit_tests
    board_test.cpp
    calendar_test.cpp
    dashboard_test.cpp
    dice_data_test.cpp
    dice_sets_test.cpp
    dice_test.cpp
    dictionary_test.cpp
    deal_test.cpp
    feedback_test.cpp
    game_rules_test.cpp
    game_session_test.cpp
    grapheme_binder_test.cpp
    haptics_test.cpp
    language_choice_test.cpp
    motion_test.cpp
    parent_gate_test.cpp
    path_test.cpp
    paths_test.cpp
    phase_data_test.cpp
    profiling_test.cpp
    scoring_test.cpp
    settings_test.cpp
    shake_test.cpp
    seeding_test.cpp
    session_store_test.cpp
    solver_test.cpp
    sound_it_out_test.cpp
    store_test.cpp
    version_test.cpp)

# Whether the profiling calls are meant to have Tracy behind them in this
# build: the option CMake was given, passed in as a value (profiling_test.cpp).
target_compile_definitions(sns_unit_tests PRIVATE
    "SNS_TEST_EXPECT_PROFILING=$<BOOL:${SNS_ENABLE_TRACY}>")

target_link_libraries(sns_unit_tests
    PRIVATE
        sns::lib
        sns::platform
        sns::warnings
        sns::sanitizers
        sns::coverage
        Catch2::Catch2WithMain)

# The shipped data, by its path in this checkout, for the cases about the data
# itself (dice_data_test.cpp), as the performance tier does for the dictionary.
target_compile_definitions(sns_unit_tests PRIVATE
    "SNS_DATA_DIRECTORY=\"${PROJECT_SOURCE_DIR}/data\"")

sns_deploy_sanitizer_runtime(sns_unit_tests)

# The unit tier. Every case is named `unit.<sentence>` and carries the label
# `unit`, so `ctest -L unit` (and the coverage-unit target) select exactly this
# tier. Catch2's own tags stay usable with the binary directly
# (`sns_unit_tests "[solver]"`); they are not CTest labels here because Catch2
# would overwrite the tier label with the tag list.
catch_discover_tests(sns_unit_tests
    TEST_PREFIX "unit."
    PROPERTIES LABELS "unit")
