// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

#include "fuzz_input.hpp"
#include "wjs/store.hpp"

namespace {

// The store file ends in "end <16 hex digits>", the FNV-1a hash of every line before it
// (libs/core/src/store_format.cpp). A mutated file almost never keeps a right sum, so the
// second pass writes one: the record parser is reached by mutations of the records and not only
// by the files whose sum happens to hold. A copy of the format's rule, here and nowhere else; if
// the format changes, the seeds stop reaching the parser and this says so as a drop in coverage.
std::string with_checksum(std::string_view text) {
    std::string body{text};
    while (!body.empty() && (body.back() == '\n' || body.back() == '\r')) {
        body.pop_back();
    }
    const std::size_t start = body.rfind('\n');
    body.resize(start == std::string::npos ? 0 : start + 1);
    std::uint64_t hash = 14695981039346656037ULL;
    for (const char c : body) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 1099511628211ULL;
    }
    constexpr std::string_view digits = "0123456789abcdef";
    std::string sum(16, '0');
    for (std::size_t i = 0; i < 16; ++i) {
        sum[15 - i] = digits[hash & 0xF];
        hash >>= 4;
    }
    return body + "end " + sum + "\n";
}

// What a store that was read must survive: written out again, it reads back as the same data.
void check_round_trip(const wjs::ParsedStore& parsed) {
    const auto again = wjs::parse_store(wjs::serialize_store(parsed.data));
    if (!again || !(again->data == parsed.data)) {
        std::abort();
    }
}

}  // namespace

// A store file as found on disk: the size, the header, the checksum, the migrations and every
// record.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto text = wjs::test::fuzz_text(data, size);
    if (const auto parsed = wjs::parse_store(text)) {
        check_round_trip(*parsed);
    }
    if (const auto parsed = wjs::parse_store(with_checksum(text))) {
        check_round_trip(*parsed);
    }
    return 0;
}
