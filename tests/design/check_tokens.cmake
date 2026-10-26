# SPDX-License-Identifier: GPL-3.0-or-later
# The design's tokens (docs/design/toybox/tokens.json and sns-tokens.json) and the Theme that spends them
# (apps/app/qml/kit/Theme.qml) say the same thing: every colour of the shared palette, the dimmer surface,
# the Night shadow, the three type faces and the smallest text size. And the font files are one list in
# four places: the files in assets/fonts, the application's resource list, the application's loader and
# the test programs' loader. A value changed in one place and not the other fails here and names both.
#
# Run as:
#   cmake -DREPO_ROOT=<checkout> -P tests/design/check_tokens.cmake

if(NOT DEFINED REPO_ROOT)
    message(FATAL_ERROR "check_tokens.cmake: pass -DREPO_ROOT=<checkout>")
endif()

set(_failures "")
function(_fail message_text)
    set(_failures "${_failures}\n  ${message_text}" PARENT_SCOPE)
endfunction()

file(READ "${REPO_ROOT}/docs/design/toybox/tokens.json" _tokens)
file(READ "${REPO_ROOT}/docs/design/toybox/sns-tokens.json" _sns)
file(READ "${REPO_ROOT}/apps/app/qml/kit/Theme.qml" _theme)

# The hex literals on the line that declares a Theme property, in order.
function(_theme_hex property out_var)
    string(REGEX MATCH "readonly property color ${property}:[^\n]*" _line "${_theme}")
    if(_line STREQUAL "")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()
    string(REGEX MATCHALL "#[0-9A-Fa-f]+" _hex "${_line}")
    set(${out_var} "${_hex}" PARENT_SCOPE)
endfunction()

function(_theme_string property out_var)
    string(REGEX MATCH "readonly property string ${property}: \"([^\"]+)\"" _m "${_theme}")
    set(${out_var} "${CMAKE_MATCH_1}" PARENT_SCOPE)
endfunction()

# --- the shared palette: tokens.json key, Theme property --------------------------------------------
set(_palette
    "background|sunshine" "ink|nightPurple" "shadow|deepPurple" "surface|cream" "accent|coral"
    "boardLight|tealLight" "boardDark|tealDark" "food|blueberry" "foodHighlight|blueberryShine"
    "leaf|leaf" "tongue|pink")
foreach(_pair IN LISTS _palette)
    string(REPLACE "|" ";" _pair "${_pair}")
    list(GET _pair 0 _key)
    list(GET _pair 1 _property)
    string(JSON _want ERROR_VARIABLE _error GET "${_tokens}" color "${_key}" value)
    if(_error)
        _fail("tokens.json has no color.${_key}")
        continue()
    endif()
    _theme_hex("${_property}" _got)
    list(LENGTH _got _count)
    if(NOT _count EQUAL 1)
        _fail("Theme.${_property} should be one hex literal (${_want}); found '${_got}'")
        continue()
    endif()
    string(TOLOWER "${_want}" _want)
    string(TOLOWER "${_got}" _got)
    if(NOT _want STREQUAL _got)
        _fail("Theme.${_property} is ${_got}, tokens.json color.${_key} is ${_want}")
    endif()
endforeach()

# --- what only this project adds (sns-tokens.json) --------------------------------------------------
string(JSON _dim GET "${_sns}" color surfaceDim value)
_theme_hex(surfaceDim _got)
list(GET _got 1 _day)
string(TOLOWER "${_dim}" _dim)
string(TOLOWER "${_day}" _day)
if(NOT _dim STREQUAL _day)
    _fail("Theme.surfaceDim (light) is ${_day}, sns-tokens.json color.surfaceDim is ${_dim}")
endif()

string(JSON _deep GET "${_sns}" color night shadowDeep value)
_theme_hex(shadow _got)
list(GET _got 0 _night)
string(TOLOWER "${_deep}" _deep)
string(TOLOWER "${_night}" _night)
if(NOT _deep STREQUAL _night)
    _fail("Theme.shadow (Night) is ${_night}, sns-tokens.json color.night.shadowDeep is ${_deep}")
endif()

# --- type -------------------------------------------------------------------------------------------
string(JSON _chrome GET "${_sns}" typography faces chrome family)
string(JSON _reading GET "${_sns}" typography faces reading family)
string(JSON _counter GET "${_sns}" typography faces counter family)
foreach(_pair "uiFont|${_chrome}" "fontFamily|${_reading}" "eyebrowFont|${_counter}")
    string(REPLACE "|" ";" _pair "${_pair}")
    list(GET _pair 0 _property)
    list(GET _pair 1 _want)
    _theme_string("${_property}" _got)
    if(_got STREQUAL "")
        # The chrome faces are the language's own Noto face in Chinese, Japanese, Hebrew and Yiddish
        # (`nativeFont`), and the token's face otherwise: it is the value after the colon that is held.
        string(REGEX MATCH "readonly property string ${_property}: nativeFont !== \"\" \\? nativeFont : \"([^\"]*)\"" _m "${_theme}")
        set(_got "${CMAKE_MATCH_1}")
    endif()
    if(NOT _want STREQUAL _got)
        _fail("Theme.${_property} is '${_got}', sns-tokens.json says '${_want}'")
    endif()
endforeach()

string(JSON _minimum GET "${_sns}" typography minimumSize)
string(REGEX MATCH "readonly property real minTextSize: ([0-9.]+)" _m "${_theme}")
if(NOT CMAKE_MATCH_1 STREQUAL _minimum)
    _fail("Theme.minTextSize is '${CMAKE_MATCH_1}', sns-tokens.json typography.minimumSize is ${_minimum}")
endif()

# --- the touch targets ------------------------------------------------------------------------------
foreach(_pair "targetMin|min" "targetChild|child" "targetPrimary|primary" "targetGap|gap")
    string(REPLACE "|" ";" _pair "${_pair}")
    list(GET _pair 0 _property)
    list(GET _pair 1 _key)
    string(JSON _want GET "${_sns}" touch "${_key}")
    string(REGEX MATCH "readonly property real ${_property}: ([0-9.]+)" _m "${_theme}")
    if(NOT CMAKE_MATCH_1 STREQUAL _want)
        _fail("Theme.${_property} is '${CMAKE_MATCH_1}', sns-tokens.json touch.${_key} is ${_want}")
    endif()
endforeach()

# --- shape: outlines, radii and hard shadows (tokens.json shape) ------------------------------------
function(_theme_number property out_var)
    string(REGEX MATCH "readonly property real ${property}: ([0-9.]+)" _m "${_theme}")
    set(${out_var} "${CMAKE_MATCH_1}" PARENT_SCOPE)
endfunction()

# Theme property | tokens.json path (keys separated by a space) | how to read the value
set(_shape
    "outlinePanel|shape outline panel|number" "outlineControl|shape outline control|number"
    "radiusXs|shape radius segment|number" "radiusSmall|shape radius chip|number"
    "radiusControl|shape radius control|number" "radius|shape radius button|number"
    "radiusPanel|shape radius panel|number" "radiusLarge|shape radius dialog|number"
    "shadowSmall|shape dropShadow small|shadow" "shadowMedium|shape dropShadow medium|shadow"
    "shadowLarge|shape dropShadow large|shadow")
foreach(_entry IN LISTS _shape)
    string(REPLACE "|" ";" _entry "${_entry}")
    list(GET _entry 0 _property)
    list(GET _entry 1 _path)
    list(GET _entry 2 _kind)
    separate_arguments(_keys UNIX_COMMAND "${_path}")
    string(JSON _raw ERROR_VARIABLE _error GET "${_tokens}" ${_keys})
    if(_error)
        _fail("tokens.json has no ${_path}")
        continue()
    endif()
    if(_kind STREQUAL "shadow")
        # "0 4px 0 #1A1035": the drop is the second number.
        string(REGEX MATCH "^0 ([0-9]+)px" _m "${_raw}")
        set(_want "${CMAKE_MATCH_1}")
    else()
        set(_want "${_raw}")
    endif()
    _theme_number("${_property}" _got)
    if(NOT _got STREQUAL _want)
        _fail("Theme.${_property} is '${_got}', tokens.json ${_path} is ${_want}")
    endif()
endforeach()

# The tray's frame: "6px ink outline plus a 12px hard drop".
string(JSON _frame GET "${_tokens}" board frame)
string(REGEX MATCH "^([0-9]+)px[^0-9]*([0-9]+)px" _m "${_frame}")
_theme_number(outlineFrame _got_outline)
_theme_number(shadowFrame _got_drop)
if(NOT _got_outline STREQUAL "${CMAKE_MATCH_1}" OR NOT _got_drop STREQUAL "${CMAKE_MATCH_2}")
    _fail("Theme.outlineFrame/shadowFrame are ${_got_outline}/${_got_drop}, tokens.json board.frame is '${_frame}'")
endif()

# --- the font files: one list in four places --------------------------------------------------------
file(GLOB _disk RELATIVE "${REPO_ROOT}/assets/fonts" "${REPO_ROOT}/assets/fonts/*.ttf")
list(SORT _disk)

function(_listed file regex out_var)
    file(READ "${REPO_ROOT}/${file}" _text)
    string(REGEX MATCHALL "${regex}" _all "${_text}")
    set(_names "")
    foreach(_one IN LISTS _all)
        string(REGEX REPLACE ".*[/\"]([A-Za-z0-9-]+[.]ttf).*" "\\1" _name "${_one}")
        list(APPEND _names "${_name}")
    endforeach()
    list(SORT _names)
    list(REMOVE_DUPLICATES _names)
    set(${out_var} "${_names}" PARENT_SCOPE)
endfunction()

_listed("apps/app/CMakeLists.txt" "assets/fonts/[A-Za-z0-9-]+[.]ttf" _cmake)
_listed("apps/app/src/main.cpp" ":/fonts/[A-Za-z0-9-]+[.]ttf" _loader)
_listed("apps/app/tests/fonts/test_fonts.cpp" "\"[A-Za-z0-9-]+[.]ttf\"" _tests)

foreach(_pair "apps/app/CMakeLists.txt|_cmake" "apps/app/src/main.cpp|_loader" "apps/app/tests/fonts/test_fonts.cpp|_tests")
    string(REPLACE "|" ";" _pair "${_pair}")
    list(GET _pair 0 _where)
    list(GET _pair 1 _variable)
    if(NOT "${${_variable}}" STREQUAL "${_disk}")
        string(REPLACE ";" ", " _have "${${_variable}}")
        string(REPLACE ";" ", " _expect "${_disk}")
        _fail("${_where} lists the fonts [${_have}], assets/fonts holds [${_expect}]")
    endif()
endforeach()

if(_failures)
    message(FATAL_ERROR "The design tokens, the Theme and the font lists disagree:${_failures}")
endif()
message(STATUS "design: the palette, the faces, the shape, the smallest text and the targets match tokens.json; the fonts are one list")
