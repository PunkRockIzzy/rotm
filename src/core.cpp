#include "core.hpp"
#include "editor.hpp"
#include "exception.hpp"
#include "gui_context.hpp"
#include "version.hpp"

#include <chrono>
#include <cmath>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <raylib.h>
#include <tracy/Tracy.hpp>

namespace rotm
{
using StartupModule = Editor;

Core::Core(int argc, char *argv[]) : logger(m_timeStart), resources(logger)
{
    logger.print_info()("RotM v{}.{}, build date " __DATE__, version::MAJOR, version::MINOR);

    logger.print_info()("Initializing...");

    try
    {
        parse_args(argc, argv);

        init_raylib();

        m_guiContext = std::make_unique<GuiContext>(*this);

        m_moduleActive = std::make_unique<StartupModule>(*this);
    }
    catch (const std::exception &)
    {
        handle_exception_init();
    }

    logger.print_info()("Finished initializing.");
}

Core::~Core()
{
    logger.print_info()("Shutting down...");

    try
    {
        m_moduleActive.reset();

        m_guiContext.reset();

        logger.print_info()("Clearing resources...");
        resources.clear();

        shutdown_raylib();
    }
    catch (const std::exception &)
    {
        handle_exception_shutdown();
    }

    logger.print_info()("Finished shutting down.");
}

void Core::run()
{
    try
    {
        while (!m_wantExit)
        {
            if (!m_moduleActive)
                throw exception::NoActiveCoreModule();

            m_deltaTime = calculate_delta_time();
            m_secondsSinceTick += m_deltaTime;

            m_timeLastFrame = time::Clock::now();

            update();

            m_wantExit = m_moduleActive->want_exit();

            if (m_wantExit)
                break;

            for (int i = 0; i < MAX_TICKS_PER_FRAME; ++i)
            {
                if (!do_tick())
                    break;

                tick();

                m_secondsSinceTick -= SECONDS_PER_TICK;
            }

            m_secondsSinceTick = std::fmod(m_secondsSinceTick, SECONDS_PER_TICK);

            draw();

            ++m_frameCountPastSecond;

            FrameMark;
        }
    }
    catch (const std::exception &)
    {
        handle_exception_run();
    }
}

void Core::update()
{
    ZoneScopedN("Core::update");

    m_moduleActive->update();
}

void Core::tick()
{
    ZoneScopedN("Core::tick");

    const bool oncePerSecond = (m_tickCount > 0) and (m_tickCount % TICKS_PER_SECOND == 0);

    m_moduleActive->tick();

    if (oncePerSecond)
    {
        const std::size_t texturesPurged = resources.purge_textures();

        if (texturesPurged)
        {
            if (texturesPurged == 1)
            {
                logger.print_info()("Purged 1 unused texture.");
            }
            else
            {
                logger.print_info()("Purged {} unused textures.", texturesPurged);
            }
        }

        m_fps = m_frameCountPastSecond;
        m_frameCountPastSecond = 0;
    }

    ++m_tickCount;
}

void Core::draw()
{
    ZoneScopedN("Core::draw");

    BeginDrawing();
    ClearBackground(BLACK);

    m_moduleActive->draw();

    m_guiContext->begin_frame();
    m_moduleActive->sync_gui();
    m_guiContext->end_frame();

    if (m_showFPSCounter)
        DrawText(TextFormat("%i FPS", m_fps), 10, 10, 20, Fade(WHITE, 0.33f));

    EndDrawing();
}

bool Core::do_tick() const
{
    return m_secondsSinceTick >= SECONDS_PER_TICK;
}

bool read_int_arg(std::string_view arg, const std::string_view &argValue, int &target)
{
    if (argValue.size() <= arg.size())
        return false;

    if (argValue[arg.size()] != '=')
        return false;

    try
    {
        const std::string param{argValue.substr(arg.size() + 1)};

        target = std::stoi(param);

        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

/*
    TODO: Low priority
    Implement a command line arg parser.

    Decide whether to use -arg=value syntax
    or -arg value syntax.

    -w or --window-width [int]
    -h or --window-height [int]
    -f or --target-fps [int]
    --rl-log-level [int]
*/
void Core::parse_args(int argc, char *argv[])
{
    using std::size_t;
    using std::string_view;
    using std::vector;

    std::vector<std::string_view> arguments{static_cast<size_t>(argc - 1)};

    for (int i = 1; i < argc; ++i) // truncate first argument, always executable path on desktop
    {
        arguments[i - 1] = argv[i];
    }

    for (size_t i = 0; i < arguments.size(); ++i)
    {
        const std::string_view &arg = arguments[i];

        if (arg == "-dev")
        {
            m_devMode = true;
            continue;
        }

        if (arg.starts_with("-fps"))
        {
            static constexpr char ERRMSG[] = "Usage: -fps=[positive integer]";

            if (!read_int_arg("-fps", arg, m_raylibConfig.targetFPS))
                logger.print_warning()(ERRMSG);
            else if (m_raylibConfig.targetFPS < 0)
                logger.print_warning()(ERRMSG);

            continue;
        }

        logger.print_warning()("Unknown command line argument. (arg={})", arg);
    }
}

void Core::init_raylib()
{
    static constexpr std::string_view logLevelNames[8] = {"LOG_ALL",     "LOG_TRACE", "LOG_DEBUG", "LOG_INFO",
                                                          "LOG_WARNING", "LOG_ERROR", "LOG_FATAL", "LOG_NONE"};

    logger.print_info()("Setting raylib log level... ({})", logLevelNames[m_raylibConfig.logLevel]);

    SetTraceLogLevel(m_raylibConfig.logLevel);

    logger.print_info()("Setting raylib window flags... ({:#010x})", m_raylibConfig.windowFlags);

    SetConfigFlags(m_raylibConfig.windowFlags);

    logger.print_info()("Creating window... (width={}, height={}, title=\"{}\")", m_raylibConfig.windowWidth,
                        m_raylibConfig.windowHeight, m_raylibConfig.windowTitle);

    InitWindow(m_raylibConfig.windowWidth, m_raylibConfig.windowHeight, m_raylibConfig.windowTitle.c_str());

    if (!IsWindowReady())
        throw exception::NoWindow();

    logger.print_info()("Setting target FPS... ({})", m_raylibConfig.targetFPS);
    SetTargetFPS(m_raylibConfig.targetFPS);

    SetExitKey(KEY_NULL);
}

void Core::shutdown_raylib()
{
    if (IsWindowReady())
    {
        logger.print_info()("Closing window...");
        CloseWindow();
    }
    else
    {
        logger.print_warning()("Window closed before shutdown_raylib called.");
    }
}

void Core::handle_exception_init()
{
    try
    {
        throw;
    }
    catch (const std::exception &e)
    {
        logger.print_error()("An unhandled exception occurred during program startup: {}", e.what());

        logger.print_info()("Attempting shutdown.");
        m_wantExit = true;
    }
}

void Core::handle_exception_run()
{
    try
    {
        throw;
    }
    catch (const std::exception &e)
    {
        logger.print_error()("An unhandled exception occurred during program running: {}", e.what());

        // control escapes while loop when exception is thrown.
        logger.print_info()("Attempting shutdown.");
    }
}

void Core::handle_exception_shutdown()
{
    try
    {
        throw;
    }
    catch (const std::exception &e)
    {
        logger.print_error()("An unhandled exception occurred during program shutdown: {}", e.what());

        logger.print_info()("Terminating.");
        std::terminate();
    }
}

double Core::calculate_delta_time()
{
    return std::chrono::duration<double>(time::since(m_timeLastFrame)).count();
}
} // namespace rotm
