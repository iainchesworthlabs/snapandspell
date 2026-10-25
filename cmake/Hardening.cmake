# SPDX-License-Identifier: GPL-3.0-or-later
# ---------------------------------------------------------------------------
# Hardening.cmake
#
# Defines an INTERFACE target `wjs::hardening`: the shared hardening set
# (icl::hardening: stack protector, _FORTIFY_SOURCE=3 outside Debug, relro and
# now, control-flow protection, /GS /guard:cf /sdl on MSVC). Link it PRIVATE-ly
# into every first-party target that ships (the libraries, the command line, the
# application and its modules); the test executables do not need it.
#
# WJS_ENABLE_HARDENING is on by default. The switch is there to bisect a
# toolchain problem, not to ship without it: no preset turns it off.
# ---------------------------------------------------------------------------

option(WJS_ENABLE_HARDENING "Compile and link the shipped targets with the hardening flags" ON)

add_library(wjs_hardening INTERFACE)
add_library(wjs::hardening ALIAS wjs_hardening)

if(WJS_ENABLE_HARDENING)
    include("${CMAKE_CURRENT_LIST_DIR}/icl/IclHardening.cmake")
    target_link_libraries(wjs_hardening INTERFACE icl::hardening)
endif()
