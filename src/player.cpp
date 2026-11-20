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

    apply_velocity_y(deltaTime);
    resolve_ground_collision(tilemap, tileSize);
    resolve_ceiling_collision(tilemap, tileSize);

    apply_velocity_x(deltaTime);
    resolve_leftwall_collision(tilemap, tileSize);
    resolve_rightwall_collision(tilemap, tileSize);
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

void Player::resolve_ceiling_collision(const Tilemap &tilemap, int tileSize)
{
    const std::optional<GlobalTileCoord> touchingCeiling = touching_ceiling(tilemap, tileSize);

    if (!touchingCeiling)
        return;

    if (velocity.y <= 0.0f)
    {
        const Vector2 tilePos = tile_to_point(*touchingCeiling, tileSize);

        velocity.y = 0.0f;
        position.y = tilePos.y + static_cast<float>(tileSize);
    }
}

void Player::resolve_leftwall_collision(const Tilemap &tilemap, int tileSize)
{
    const std::optional<GlobalTileCoord> touchingLeftWall = touching_leftwall(tilemap, tileSize);

    if (!touchingLeftWall)
        return;

    if (velocity.x <= 0.0f)
    {
        const Vector2 tilePos = tile_to_point(*touchingLeftWall, tileSize);

        velocity.x = 0.0f;
        position.x = tilePos.x + static_cast<float>(tileSize);
    }
}

void Player::resolve_rightwall_collision(const Tilemap &tilemap, int tileSize)
{
    const std::optional<GlobalTileCoord> touchingRightWall = touching_rightwall(tilemap, tileSize);

    if (!touchingRightWall)
        return;

    if (velocity.x >= 0.0f)
    {
        const Rectangle rec = get_rec(position);
        const Vector2 tilePos = tile_to_point(*touchingRightWall, tileSize);

        velocity.x = 0.0f;
        position.x = tilePos.x - rec.width;
    }
}
} // namespace rotm
