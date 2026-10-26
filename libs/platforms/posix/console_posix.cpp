// SPDX-License-Identifier: GPL-3.0-or-later
#include "sns/platform/console.hpp"

namespace sns::platform {

// A POSIX terminal carries bytes, and the programs here write UTF-8: there is
// nothing to switch on.
void use_utf8_console() {}

}  // namespace sns::platform
