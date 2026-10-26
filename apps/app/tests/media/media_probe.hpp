// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Whether Qt Multimedia has a backend on this machine, for the tests that show the camera's live
// view. The VideoOutput the preview is made of needs one: a kit whose backend plugin cannot load
// (a missing system library) answers "No QtMultimedia backends found" and every VideoOutput
// fails to create its sink. Such a test is skipped there, not failed: the screens are not what
// is missing. A kit built without Qt Multimedia always answers false.
//
// The backend's own start-up messages (the optional libraries it did not find, and that it has
// no backend at all) are printed once, here, by Qt's default handler. They are neither counted
// as the preview's warnings nor failures of the test function that happened to ask first.
//
// Test support: nothing in libs and apps link it. The Qt Multimedia half and the none half are
// two files; apps/app/tests/media/CMakeLists.txt compiles the one the kit calls for.

namespace sns::app::test {

[[nodiscard]] bool mediaBackendAvailable();

}  // namespace sns::app::test
