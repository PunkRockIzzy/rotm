#pragma once

#include "core_module.hpp"
#include "gui_context.hpp"
#include "logger.hpp"
#include "resource_manager.hpp"
#include "time.hpp"
#include "version.hpp"

#include <deque>
#include <format>
#include <memory>
#include <string>

namespace rotm
{
class Core
{
public: /* Types */
    struct RaylibConfig
    {
        std::string windowTitle = std::format("RotM v{}.{}", version::MAJOR, version::MINOR);

        unsigned windowFlags = 0x00000004; // FLAG_WINDOW_RESIZABLE
        int windowWidth = 1280;
        int windowHeight = 720;
        int targetFPS = 120;
        int logLevel = 4; // LOG_WARNING
    };

public:
    static constexpr int TICKS_PER_SECOND = 60;

public: /* Methods */
    Core(int argc, char* argv[]);
    ~Core();

    void run();

    const time::Point& get_time_start() const
    {
        return m_timeStart;
    }

    bool is_dev_mode() const
    {
        return m_devMode;
    }

    void set_dev_mode(bool enabled)
    {
        m_devMode = enabled;
    }

    double get_delta_time() const
    {
        return m_deltaTime;
    }

    unsigned long long get_tick_count() const
    {
        return m_tickCount;
    }

    int get_fps() const
    {
        return m_fps;
    }

    void set_target_fps(int targetFPS)
    {
        m_raylibConfig.targetFPS = targetFPS;
    }

    bool get_show_fps_counter() const
    {
        return m_showFPSCounter;
    }

    void set_show_fps_counter(bool enabled)
    {
        m_showFPSCounter = enabled;
    }

    const RaylibConfig& get_raylib_config() const
    {
        return m_raylibConfig;
    }

    void request_module_transition(std::unique_ptr<CoreModule>&& module)
    {
        m_moduleQueued = std::move(module);
    }

private: /* Static properties */
    static constexpr double SECONDS_PER_TICK = 1.0 / static_cast<double>(TICKS_PER_SECOND);
    static constexpr int MAX_TICKS_PER_FRAME = 4;
    static constexpr std::size_t DELTA_HISTORY_MAX = 100;

private: /* Methods */
    void update();
    void tick();
    void draw();

    bool do_tick() const;

    void wait_for_next_frame();

    void parse_args(int argc, char* argv[]);
    void init_raylib();
    void shutdown_raylib();

    void handle_exception_init();
    void handle_exception_run();
    void handle_exception_shutdown();

    double calculate_delta_time() const;
    int calculate_fps() const;

    std::string generate_screenshot_filename();

private: /* Fields */
    const time::Point m_timeStart = time::Clock::now();
    time::Point m_timeLastFrame = m_timeStart;

    bool m_devMode = false;

    double m_deltaTime = 0.0;
    std::deque<double> m_deltaHistory;
    double m_deltaHistorySum = 0.0;

    int m_fps = 0;
    bool m_showFPSCounter = false;

    double m_secondsSinceTick = 0.0;
    unsigned long long m_tickCount = 0;

    std::size_t m_busyCounter = 0;

    int m_lastWindowWidth = 0;
    int m_lastWindowHeight = 0;
    bool m_isFullscreen = false;

    std::unique_ptr<CoreModule> m_moduleActive;
    std::unique_ptr<CoreModule> m_moduleQueued;

    bool m_wantExit = false;
    RaylibConfig m_raylibConfig = RaylibConfig{};

public: /* Fields */
    Logger logger;
    ResourceManager resources;
    std::unique_ptr<GuiContext> guiContext;
};
} // namespace rotm
