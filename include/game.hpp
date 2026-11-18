#pragma once

#include "core_module.hpp"
#include "player.hpp"
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
    void update_player();

    void update_camera();
    static Vector2 get_camera_offset();

  private: /* Fields */
    std::unique_ptr<Scene> m_scene;

    Player m_player;
};
} // namespace rotm
