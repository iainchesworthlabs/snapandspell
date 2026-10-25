// SPDX-License-Identifier: GPL-3.0-or-later
#include "media_probe.hpp"

namespace wjs::app::test {

// A kit built without Qt Multimedia has no backend to ask about.
bool mediaBackendAvailable() {
    return false;
}

}  // namespace wjs::app::test
