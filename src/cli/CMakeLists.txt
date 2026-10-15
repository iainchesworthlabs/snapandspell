add_executable(wjs_cli main.cpp)
set_target_properties(wjs_cli PROPERTIES OUTPUT_NAME wjs-cli)

target_link_libraries(wjs_cli
    PRIVATE
        wjs::lib
        wjs::platform
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)

wjs_deploy_sanitizer_runtime(wjs_cli)
