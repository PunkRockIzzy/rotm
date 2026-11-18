#include "player.hpp"
#include "raylib.h"

#include <memory>

namespace rotm
{
Player::Player(Vector2 startPos, std::shared_ptr<TextureResource> &&spriteTexture)
    : SpriteActor(startPos, std::move(spriteTexture))
{
}

void Player::update(double deltaTime)
{
    if (IsKeyDown(KEY_D))
    {
        velocity.x = 50.0f;
    }
    else if (IsKeyDown(KEY_A))
    {
        velocity.x = -50.0f;
    }
    else
    {
        velocity.x = 0.0f;
    }

    SpriteActor::update(deltaTime);
}
} // namespace rotm
