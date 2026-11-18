#pragma once

#include "actor.hpp"
#include "core_module.hpp"
#include "scene.hpp"

namespace rotm
{
class Game final : public CoreModule
{
  public: /* Methods */
    Game(Core &coreRef);
    ~Game() override;

    void update() override;
    void draw() override;

  private: /* Methods */
    static Vector2 get_camera_offset();

  private: /* Fields */
    std::unique_ptr<Scene> m_scene;

    SpriteActor m_player;
};
} // namespace rotm
