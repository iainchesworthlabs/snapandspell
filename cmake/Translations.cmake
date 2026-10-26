# SPDX-License-Identifier: GPL-3.0-or-later
# ---------------------------------------------------------------------------
# The interface's catalogues (docs/design/application.md, "Languages").
#
# One Qt Linguist .ts catalogue per language in apps/app/translations/, checked in. lupdate scans
# the QML sources (and any C++ that carries qsTr) into them; lrelease compiles
# them into .qm files at build time, and they are embedded in the sns_i18n
# library's resources as :/i18n/sns_<code>.qm, which LanguageManager loads.
#
#   cmake --build <build> --target sns_update_translations
#
# rewrites the catalogues from the sources. The test i18n.catalogues regenerates
# them into a temporary directory and fails when they differ, so a qsTr() added
# without its catalogue is caught (docs/ci.md). The catalogues are generated from
# the sources and are not edited by hand except for the translations themselves.
#
# The source list is explicit because the QML lives in several modules under
# apps/app/qml (the application's shell and kit, and the parent, scan and scene
# modules): one directory covers them today, and a module added outside it must
# be added here.
# ---------------------------------------------------------------------------

get_filename_component(SNS_TRANSLATION_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# English is the source language, and it has a catalogue too, so that every
# check treats the eight alike and the completeness test compares against one of
# them.
set(SNS_TRANSLATION_LANGUAGES en fr es de yi he zh_CN ja)

set(SNS_TRANSLATION_SOURCES
    apps/app/qml
    apps/app/src/main.cpp)

set(SNS_TS_FILES "")
foreach(sns_language IN LISTS SNS_TRANSLATION_LANGUAGES)
    list(APPEND SNS_TS_FILES
        "${SNS_TRANSLATION_ROOT}/apps/app/translations/sns_${sns_language}.ts")
endforeach()

# Compile this target's languages and embed them, and add the target that
# regenerates the catalogues. Call it with the library the resources belong to
# (sns_i18n), after Qt6LinguistTools has been found.
function(sns_add_translations target)
    if(NOT TARGET Qt6::lrelease OR NOT TARGET Qt6::lupdate)
        message(FATAL_ERROR
            "The interface's catalogues need Qt's Linguist tools "
            "(Qt6::lrelease and Qt6::lupdate). Install the Qt 'qttools' module "
            "or configure with -DSNS_BUILD_APP=OFF. See docs/building.md.")
    endif()

    set(qm_directory "${CMAKE_CURRENT_BINARY_DIR}/i18n")
    qt_add_lrelease(
        TS_FILES ${SNS_TS_FILES}
        QM_OUTPUT_DIRECTORY "${qm_directory}"
        QM_FILES_OUTPUT_VARIABLE sns_qm_files
        LRELEASE_TARGET sns_lrelease)
    # qt_add_lrelease compiles them; the resource must know that the .qm files
    # are build products and not sources in the tree.
    set_source_files_properties(${sns_qm_files} PROPERTIES GENERATED TRUE)
    qt_add_resources(${target} sns_translations
        PREFIX "/i18n"
        BASE "${qm_directory}"
        FILES ${sns_qm_files})

    add_custom_target(sns_update_translations
        COMMAND "$<TARGET_FILE:Qt6::lupdate>"
            -locations relative
            -no-obsolete
            ${SNS_TRANSLATION_SOURCES}
            -ts ${SNS_TS_FILES}
        WORKING_DIRECTORY "${SNS_TRANSLATION_ROOT}"
        COMMENT "Updating the interface catalogues in apps/app/translations/ from the sources"
        VERBATIM)
endfunction()
