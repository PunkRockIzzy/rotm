#pragma once

#include "core_module.hpp"

namespace rotm
{
class Game final : public CoreModule
{
  public:
    Game(Core &coreRef);
    ~Game() override;

    void update() override;
    void draw() override;
};
} // namespace rotm
