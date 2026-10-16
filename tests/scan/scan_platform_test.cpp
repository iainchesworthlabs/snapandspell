// The camera and board_reader interfaces: what any implementation of them, on any
// platform, has to keep true. Nothing here names a platform: where a platform has no
// camera (the desktops today) the contract says so with `unavailable`, and when one
// gets a camera these same cases hold it.

#include <catch2/catch_test_macros.hpp>

#include "fixtures.hpp"
#include "wjs/platform/board_reader.hpp"
#include "wjs/platform/camera.hpp"

namespace {

namespace camera = wjs::platform::camera;
namespace board_reader = wjs::platform::board_reader;
using namespace wjs::scan;
using namespace wjs::scan::synth;

}  // namespace

TEST_CASE("a camera that was never started is not capturing and has no frame", "[scan][platform]") {
    camera::stop();  // safe with nothing running
    CHECK_FALSE(camera::is_capturing());
    CHECK_FALSE(camera::try_pop_latest_frame().has_value());
    camera::stop();  // and twice
    CHECK_FALSE(camera::is_capturing());
}

TEST_CASE("the cameras are listed or the platform says it cannot", "[scan][platform]") {
    const auto devices = camera::enumerate_devices();
    if (devices.has_value()) {
        for (const camera::DeviceInfo& device : *devices) {
            CHECK_FALSE(device.identifier.empty());
        }
    } else {
        CHECK((devices.error() == camera::CameraError::unavailable ||
               devices.error() == camera::CameraError::permission_denied ||
               devices.error() == camera::CameraError::failed));
    }
}

TEST_CASE("a camera that is not listed cannot be started", "[scan][platform]") {
    camera::DeviceInfo unknown;
    unknown.identifier = "no-such-camera";
    unknown.display_name = "No such camera";
    const auto started = camera::start(unknown);
    REQUIRE_FALSE(started.has_value());
    CHECK(started.error() != camera::CameraError::already_running);
    // A failed start leaves nothing running and nothing to take.
    CHECK_FALSE(camera::is_capturing());
    CHECK_FALSE(camera::try_pop_latest_frame().has_value());
    camera::stop();
}

TEST_CASE("sending the person to the settings is always safe to ask", "[scan][platform]") {
    camera::open_permission_settings();  // nothing to open on a desktop; never throws
    SUCCEED();
}

TEST_CASE("each camera error says what is wrong", "[scan][platform]") {
    for (const camera::CameraError error :
         {camera::CameraError::unavailable, camera::CameraError::permission_denied,
          camera::CameraError::no_such_device, camera::CameraError::already_running,
          camera::CameraError::failed}) {
        CHECK_FALSE(camera::to_string(error).empty());
    }
}

TEST_CASE("the board reader has a name and a kind", "[scan][platform]") {
    CHECK_FALSE(board_reader::name().empty());
    const auto kind = board_reader::kind();
    CHECK((kind == board_reader::Kind::portable || kind == board_reader::Kind::native));
}

TEST_CASE("the portable board reader is the library's reader, unchanged", "[scan][platform]") {
    if (board_reader::kind() != board_reader::Kind::portable) {
        SKIP("this platform reads boards with its own recogniser, which has its own tests");
    }
    Spec spec;
    spec.condition = Condition::upright;
    spec.split = Split::tuning;
    spec.size = 4;
    spec.index = 2;
    const Rendered& frame = fixtures::frame(spec);
    const auto& direct = fixtures::reading_of(spec);
    REQUIRE(direct.has_value());

    const auto through = board_reader::read(frame.image.view());
    REQUIRE(through.has_value());
    REQUIRE(through->tiles.size() == direct->tiles.size());
    for (std::size_t i = 0; i < direct->tiles.size(); ++i) {
        CHECK(through->tiles[i].letters == direct->tiles[i].letters);
        CHECK(through->tiles[i].doubtful == direct->tiles[i].doubtful);
    }

    // Its errors are the library's.
    CHECK(board_reader::read(Frame{}).error() == ReadError::invalid_frame);
    const GrayImage blank(320, 240, 100);
    CHECK(board_reader::read(blank.view()).error() == ReadError::board_not_found);
}

TEST_CASE("a frame the camera hands out is a frame the reader can be given", "[scan][platform]") {
    // The mailbox gives an owned frame; its view is what the reader takes.
    OwnedFrame owned;
    owned.width = 4;
    owned.height = 2;
    owned.format = PixelFormat::gray8;
    owned.pixels.assign(8, 90);
    CHECK(owned.view().valid());
    CHECK(owned.view().width == 4);
    CHECK(owned.view().pixels.size() == 8);
    CHECK(board_reader::read(owned.view(), {}).error() == ReadError::board_not_found);
}
