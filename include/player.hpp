#pragma once

#include "actor.hpp"
#include "tilemap.hpp"

#include <memory>

namespace rotm
{
class Player final : public SpriteActor
{
  public:
    Player(Vector2 startPos, std::shared_ptr<TextureResource> &&spriteTexture);
    ~Player() override = default;

    void update(double deltaTime, const Tilemap &tilemap, int tileSize) override;

  private:
    void resolve_ground_collision(const Tilemap &tilemap, int tileSize);
    void resolve_ceiling_collision(const Tilemap &tilemap, int tileSize);
    void resolve_leftwall_collision(const Tilemap &tilemap, int tileSize);
    void resolve_rightwall_collision(const Tilemap &tilemap, int tileSize);
};
} // namespace rotm
