# The application's feedback engines: the sound, the platform's voice and the
# device's motion, behind the seams the screens already call.
#
# Each engine is a small QObject whose backend is one of two sources with the
# same header path, chosen here by CMake:
#
#   wjs/app/sound_backend.hpp   variants/sound-multimedia  or  sound-silent
#   wjs/app/speech_backend.hpp  variants/speech-texttospeech or speech-silent
#   wjs/app/motion_backend.hpp  variants/motion-sensors    or  motion-silent
#
# It is the repository's variant pattern (cmake/Tracy.cmake and
# libs/core/variants/profiling-*): the module is found optionally by the
# top-level CMakeLists.txt, one directory is put on the include path, and no
# source asks whether the module is there. A contributor's kit without the
# three modules still configures, builds and plays, silently.
add_library(wjs_feedback STATIC
    feedback_engine.cpp
    feedback_engine.hpp
    motion_source.cpp
    motion_source.hpp
    sound_engine.cpp
    sound_engine.hpp
    sound_sequence_player.cpp
    sound_sequence_player.hpp
    speech.cpp
    speech.hpp
    "${WJS_SOUND_VARIANT_DIR}/wjs/app/sound_backend.cpp"
    "${WJS_SPEECH_VARIANT_DIR}/wjs/app/speech_backend.cpp"
    "${WJS_MOTION_VARIANT_DIR}/wjs/app/motion_backend.cpp")
set_target_properties(wjs_feedback PROPERTIES AUTOMOC ON)

target_include_directories(wjs_feedback
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/apps/app/src"
    PRIVATE
        "${WJS_SOUND_VARIANT_DIR}"
        "${WJS_SPEECH_VARIANT_DIR}"
        "${WJS_MOTION_VARIANT_DIR}")

target_link_libraries(wjs_feedback
    PUBLIC
        wjs::lib
        Qt6::Core
    PRIVATE
        wjs::platform
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)

# The optional modules are linked only where they were found: the silent
# variants then have no Qt Multimedia, Sensors or TextToSpeech to load.
if(WJS_QT_HAS_MULTIMEDIA)
    target_link_libraries(wjs_feedback PRIVATE Qt6::Multimedia)
endif()
if(WJS_QT_HAS_SENSORS)
    target_link_libraries(wjs_feedback PRIVATE Qt6::Sensors)
endif()
if(WJS_QT_HAS_TEXTTOSPEECH)
    target_link_libraries(wjs_feedback PRIVATE Qt6::TextToSpeech)
endif()
