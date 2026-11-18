#pragma once

#include "imgui.h"

namespace rotm
{
class Core;
class Logger;

class GuiContext
{
  public: /* Fonts */
    struct Fonts
    {
        ImFont *regular = nullptr;
        ImFont *regularSmall = nullptr;
        ImFont *monospace = nullptr;
        ImFont *monospaceSmall = nullptr;
    };

  public: /* Methods */
    GuiContext(Core &coreRef);
    ~GuiContext();

    void begin_frame();
    void end_frame();

    const Fonts &get_fonts() const
    {
        return m_fonts;
    }

  private: /* Static properties */
    static constexpr bool USE_DARK_THEME = true;

  private: /* Fields */
    Core &core;
    Logger &logger;

    ImGuiIO *io = nullptr;

    Fonts m_fonts;
};
} // namespace rotm
