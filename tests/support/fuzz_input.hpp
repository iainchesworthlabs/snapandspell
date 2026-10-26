// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

// What the libFuzzer harnesses (libs/<lib>/fuzz) share: the bytes libFuzzer hands over, as the
// text the readers take. Test support: nothing in libs and apps link it.

namespace sns::test {

[[nodiscard]] inline std::string_view fuzz_text(const std::uint8_t* data,
                                                std::size_t size) noexcept {
    return {reinterpret_cast<const char*>(data), size};
}

[[nodiscard]] inline std::string fuzz_string(const std::uint8_t* data, std::size_t size) {
    return std::string(fuzz_text(data, size));
}

}  // namespace sns::test
