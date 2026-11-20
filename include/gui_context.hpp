#pragma once

#include "logger.hpp"

#include <array>

#include <imgui.h>

namespace rotm
{
class Core;
class Logger;

class GuiContext
{
  public: /* Types */
    struct Fonts
    {
        ImFont *regular = nullptr;
        ImFont *regularSmall = nullptr;
        ImFont *monospace = nullptr;
        ImFont *monospaceSmall = nullptr;
    };

    // Holds variables related to the log window.
    struct WindowLog
    {
        // Should the log window be shown?
        bool active = false;
        // Should the log messages be color-coded based on level?
        bool showColors = true;
        // Should we use a larger font size for text in this window?
        bool useLargeFont = false;
        // What log levels should be shown in the window?
        std::array<bool, std::to_underlying(Logger::Level::Count)> showLevels = {
            false, // All (not a real log level)
            false, // Debug (ignored to avoid spam)
            true,  // Info
            true,  // Warning
            true,  // Error
            false  // None (not a real log level)
        };
    };

  public: /* Methods */
    GuiContext(Core &coreRef);
    ~GuiContext();

    void begin_frame();
    void end_frame();

    void window_log();

    const Fonts &get_fonts() const
    {
        return m_fonts;
    }

  public: /* Fields */
    WindowLog windowLog;

  private: /* Static properties */
    static constexpr bool USE_DARK_THEME = true;

    static constexpr ImVec2 LOG_SIZE_DEFAULT = {640.0f, 480.0f};

  private: /* Methods */
    void window_log_menubar();
    void window_log_table();

  private: /* Fields */
    Core &core;
    Logger &logger;

    ImGuiIO *io = nullptr;

    Fonts m_fonts;
};
} // namespace rotm
