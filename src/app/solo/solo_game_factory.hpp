#pragma once

#include <memory>

#include "solo_game.hpp"

namespace wjs::app {

// The game the application plays. Today FakeSoloGame: the library's session
// (game_session.hpp, the game-session work) is not on develop yet, and wiring it
// is a change to this one function and one adapter class. See solo_game.hpp.
[[nodiscard]] std::unique_ptr<SoloGame> make_solo_game();

}  // namespace wjs::app
