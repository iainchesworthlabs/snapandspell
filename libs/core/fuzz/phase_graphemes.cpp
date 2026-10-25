// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdint>
#include <sstream>
#include <string>

#include "fuzz_input.hpp"
#include "wjs/phase_data.hpp"

// A phonics graphemes file (data/phonics/<phase>.graphemes): every problem it has is reported, none
// thrown, and each problem can be written out.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    std::istringstream in{wjs::test::fuzz_string(data, size)};
    const auto parsed = wjs::parse_graphemes(in, "fuzz.graphemes");
    if (!parsed) {
        for (const auto& problem : parsed.error()) {
            static_cast<void>(wjs::to_string(problem));
        }
    }
    return 0;
}
