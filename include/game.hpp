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

  public:
    struct Constants
    {
        static constexpr float GRAVITY = 1000.0f;
        static constexpr float TERMINAL_VELOCITY = 500.0f;
        static constexpr float JUMP_FORCE = 500.0f;
    };

  private: /* Methods */
    void update_camera();
    static Vector2 get_camera_offset();

  private: /* Fields */
    std::unique_ptr<Scene> m_scene;

    Player m_player;
};
} // namespace rotm
