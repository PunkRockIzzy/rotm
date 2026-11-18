#include "game.hpp"

#include "core.hpp"
#include "raylib.h"

namespace rotm
{
Game::Game(Core &coreRef) : CoreModule(coreRef)
{
}

Game::~Game()
{
}

void Game::update()
{
}

void Game::draw()
{
    DrawText("game goes here", 0, 0, 40, WHITE);
}
} // namespace rotm
