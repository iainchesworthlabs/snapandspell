#include "solo_game_factory.hpp"

#include "fake_solo_game.hpp"

namespace wjs::app {

std::unique_ptr<SoloGame> make_solo_game() {
    return std::make_unique<FakeSoloGame>();
}

}  // namespace wjs::app
