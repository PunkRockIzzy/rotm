#include "player.hpp"
#include "game.hpp"
#include "tile_math.hpp"
#include "tilemap.hpp"

#include <algorithm>
#include <memory>

#include <raylib.h>

namespace rotm
{
Player::Player(Vector2 startPos, std::shared_ptr<TextureResource> &&spriteTexture)
    : SpriteActor(startPos, std::move(spriteTexture))
{
}

void Player::update(double deltaTime, const Tilemap &tilemap, int tileSize)
{
    if (IsKeyDown(KEY_D))
    {
        velocity.x = 100.0f;
        sprite.flipH = false;
    }
    else if (IsKeyDown(KEY_A))
    {
        velocity.x = -100.0f;
        sprite.flipH = true;
    }
    else
    {
        velocity.x = 0.0f;
    }

    const std::optional<GlobalTileCoord> touchingGround = touching_ground(tilemap, tileSize);

    if (touchingGround)
    {
        if (IsKeyPressed(KEY_SPACE))
        {
            velocity.y = -Game::Constants::JUMP_FORCE;
        }
    }
    else
    {
        velocity.y += Game::Constants::GRAVITY * deltaTime;
        velocity.y = std::min(velocity.y, Game::Constants::TERMINAL_VELOCITY);
    }

    SpriteActor::update(deltaTime, tilemap, tileSize);

    resolve_ground_collision(tilemap, tileSize);
}

void Player::resolve_ground_collision(const Tilemap &tilemap, int tileSize)
{
    const std::optional<GlobalTileCoord> touchingGround = touching_ground(tilemap, tileSize);

    if (!touchingGround)
        return;

    if (velocity.y >= 0.0f)
    {
        const Rectangle rec = get_rec(position);
        const Vector2 tilePos = tile_to_point(*touchingGround, tileSize);

        velocity.y = 0.0f;

        position.y = tilePos.y - rec.height;
    }
}
} // namespace rotm
