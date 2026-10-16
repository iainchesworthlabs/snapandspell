#include "solo_game_factory.hpp"

#include <utility>

#include "fake_solo_game.hpp"

namespace wjs::app {
namespace {

SoloGameMaker& installed() {
    static SoloGameMaker maker;
    return maker;
}

}  // namespace

ScopedSoloGameMaker::ScopedSoloGameMaker(SoloGameMaker maker) {
    installed() = std::move(maker);
}

ScopedSoloGameMaker::~ScopedSoloGameMaker() {
    installed() = nullptr;
}

std::unique_ptr<SoloGame> make_solo_game() {
    if (const SoloGameMaker& maker = installed()) {
        return maker();
    }
    return std::make_unique<FakeSoloGame>();
}

}  // namespace wjs::app
