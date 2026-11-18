#pragma once

#include "actor.hpp"

#include <memory>

namespace rotm
{
class Player final : public SpriteActor
{
  public:
    Player(Vector2 startPos, std::shared_ptr<TextureResource> &&spriteTexture);
    ~Player() override = default;

    void update(double deltaTime) override;
};
} // namespace rotm
