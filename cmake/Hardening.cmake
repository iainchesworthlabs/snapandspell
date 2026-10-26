# SPDX-License-Identifier: GPL-3.0-or-later
# ---------------------------------------------------------------------------
# Hardening.cmake
#
# Defines an INTERFACE target `sns::hardening`: the shared hardening set
# (icl::hardening: stack protector, _FORTIFY_SOURCE=3 outside Debug, relro and
# now, control-flow protection, /GS /guard:cf /sdl on MSVC). Link it PRIVATE-ly
# into every first-party target that ships (the libraries, the command line, the
# application and its modules); the test executables do not need it.
#
# SNS_ENABLE_HARDENING is on by default. The switch is there to bisect a
# toolchain problem, not to ship without it: only the linux-fuzz preset turns it off, because a
# fortified libc call bypasses AddressSanitizer's interceptors.
# ---------------------------------------------------------------------------

option(SNS_ENABLE_HARDENING "Compile and link the shipped targets with the hardening flags" ON)

add_library(sns_hardening INTERFACE)
add_library(sns::hardening ALIAS sns_hardening)

if(SNS_ENABLE_HARDENING)
    include("${CMAKE_CURRENT_LIST_DIR}/icl/IclHardening.cmake")
    target_link_libraries(sns_hardening INTERFACE icl::hardening)
endif()
