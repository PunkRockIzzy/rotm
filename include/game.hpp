#pragma once

#include "actor.hpp"
#include "core_module.hpp"
#include "scene.hpp"

namespace rotm
{
class Game final : public CoreModule
{
  public:
    Game(Core &coreRef);
    ~Game() override;

    void update() override;
    void draw() override;

  private:
    std::unique_ptr<Scene> m_scene;

    SpriteActor m_player;
};
} // namespace rotm
