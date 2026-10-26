// SPDX-License-Identifier: GPL-3.0-or-later
#include <windows.h>

#include "sns/platform/console.hpp"

namespace sns::platform {

void use_utf8_console() {
    ::SetConsoleOutputCP(CP_UTF8);
    ::SetConsoleCP(CP_UTF8);
}

}  // namespace sns::platform
