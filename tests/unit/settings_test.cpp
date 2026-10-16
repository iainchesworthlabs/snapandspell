#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "wjs/settings.hpp"

TEST_CASE("the defaults are the table's", "[settings]") {
    const wjs::Settings s;

    CHECK(s.help);
    CHECK(s.board == wjs::BoardMode::standard);
    CHECK(s.phase == "A");
    CHECK(s.focus_sounds.empty());
    CHECK(s.board_size == 3);
    CHECK(s.sound_it_out == wjs::SoundItOut::full);
    CHECK(s.celebration_sounds == wjs::CelebrationSounds::full);
    CHECK(s.sound);
    CHECK(s.vibration);
    CHECK(s.shake);
    CHECK(s.motion == wjs::MotionSetting::system);
    CHECK(s.text_size_percent == 100);
    CHECK(s.school_font.empty());
}

TEST_CASE("the defaults are already normal", "[settings]") {
    const wjs::Settings s;
    CHECK(s.normalized() == s);
}

TEST_CASE("every value the screens offer survives normalisation", "[settings]") {
    wjs::Settings s;
    s.help = false;
    s.board = wjs::BoardMode::phonics;
    s.phase = "B";
    s.focus_sounds = {"ai", "ee"};
    s.board_size = 4;
    s.sound_it_out = wjs::SoundItOut::word_only;
    s.celebration_sounds = wjs::CelebrationSounds::quiet;
    s.sound = false;
    s.vibration = false;
    s.shake = false;
    s.motion = wjs::MotionSetting::reduced;
    s.text_size_percent = 150;
    s.school_font = "Some-Font.1";

    CHECK(s.normalized() == s);
}

TEST_CASE("a value outside the table becomes the default for it", "[settings]") {
    wjs::Settings s;
    s.board_size = 9;
    s.text_size_percent = 130;
    s.phase = "not valid!";
    s.school_font = "-";
    s.board = static_cast<wjs::BoardMode>(7);
    s.sound_it_out = static_cast<wjs::SoundItOut>(-1);
    s.celebration_sounds = static_cast<wjs::CelebrationSounds>(99);
    s.motion = static_cast<wjs::MotionSetting>(5);

    const wjs::Settings n = s.normalized();

    CHECK(n.board_size == 3);
    CHECK(n.text_size_percent == 100);
    CHECK(n.phase == "A");
    CHECK(n.school_font.empty());
    CHECK(n.board == wjs::BoardMode::standard);
    CHECK(n.sound_it_out == wjs::SoundItOut::full);
    CHECK(n.celebration_sounds == wjs::CelebrationSounds::full);
    CHECK(n.motion == wjs::MotionSetting::system);
}

TEST_CASE("a board size is 3 or 4 and nothing else", "[settings]") {
    for (int size = -2; size <= 8; ++size) {
        wjs::Settings s;
        s.board_size = size;
        const int got = s.normalized().board_size;
        CHECK(((size == 3 || size == 4) ? got == size : got == 3));
    }
}

TEST_CASE("a text size is 100, 125 or 150 and nothing else", "[settings]") {
    for (const int percent : {0, 99, 100, 101, 124, 125, 126, 149, 150, 151, 200}) {
        wjs::Settings s;
        s.text_size_percent = percent;
        const int got = s.normalized().text_size_percent;
        CHECK(((percent == 100 || percent == 125 || percent == 150) ? got == percent : got == 100));
    }
}

TEST_CASE("a phase is upper cased and must be a name", "[settings]") {
    wjs::Settings s;
    s.phase = "b";
    CHECK(s.normalized().phase == "B");
    s.phase = "";
    CHECK(s.normalized().phase == "A");
    s.phase = std::string(wjs::max_name_length + 1, 'a');
    CHECK(s.normalized().phase == "A");
    s.phase = std::string(wjs::max_name_length, 'a');
    CHECK(s.normalized().phase == std::string(wjs::max_name_length, 'A'));
    s.phase = "A B";
    CHECK(s.normalized().phase == "A");
}

TEST_CASE("focus sounds are lower case, valid, sorted and each once", "[settings]") {
    wjs::Settings s;
    s.focus_sounds = {"EE", "ai", "ai", "a_e", "not a sound", "", "ay"};

    const auto n = s.normalized();

    CHECK(n.focus_sounds == std::vector<std::string>{"a_e", "ai", "ay", "ee"});
}

TEST_CASE("there are no more focus sounds than the bound", "[settings]") {
    wjs::Settings s;
    for (int i = 0; i < 200; ++i) {
        s.focus_sounds.push_back("s" + std::to_string(i));
    }

    CHECK(s.normalized().focus_sounds.size() == wjs::max_focus_sounds);
}

TEST_CASE("a school font is a plain name or none", "[settings]") {
    wjs::Settings s;
    s.school_font = "Edu.NSW-Foundation_2";
    CHECK(s.normalized().school_font == "Edu.NSW-Foundation_2");
    s.school_font = "has space";
    CHECK(s.normalized().school_font.empty());
    s.school_font = "../path";
    CHECK(s.normalized().school_font.empty());
    s.school_font = std::string(wjs::max_font_name_length + 1, 'a');
    CHECK(s.normalized().school_font.empty());
}

TEST_CASE("each enum is written by one name and read back by it", "[settings]") {
    for (const auto v : {wjs::BoardMode::standard, wjs::BoardMode::phonics}) {
        CHECK(wjs::board_mode_from_string(wjs::to_string(v)) == v);
    }
    for (const auto v : {wjs::SoundItOut::full, wjs::SoundItOut::word_only, wjs::SoundItOut::off}) {
        CHECK(wjs::sound_it_out_from_string(wjs::to_string(v)) == v);
    }
    for (const auto v : {wjs::CelebrationSounds::full, wjs::CelebrationSounds::quiet,
                         wjs::CelebrationSounds::off}) {
        CHECK(wjs::celebration_sounds_from_string(wjs::to_string(v)) == v);
    }
    for (const auto v : {wjs::MotionSetting::system, wjs::MotionSetting::reduced}) {
        CHECK(wjs::motion_from_string(wjs::to_string(v)) == v);
    }
    CHECK_FALSE(wjs::board_mode_from_string("Standard").has_value());
    CHECK_FALSE(wjs::motion_from_string("").has_value());
}
