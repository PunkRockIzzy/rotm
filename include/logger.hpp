#pragma once

#include "time.hpp"
#include "utility.hpp"

#include <chrono>
#include <deque>
#include <format>
#include <print>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

namespace rotm
{
class Logger
{
public: /* Types */
    enum class Level : unsigned short
    {
        All = 0u,
        Debug,
        Info,
        Warning,
        Error,
        None,
        Count
    };

    struct LogProxy
    {
        Logger& logger;
        Level level;
        std::source_location location;

        template <typename... Args> void operator()(std::format_string<Args...> message, Args&&... args) const
        {
            logger.print_impl(level, location, message, std::forward<Args>(args)...);
        }
    };

    struct MsgData
    {
        std::string text;
        std::chrono::hh_mm_ss<std::chrono::milliseconds> time;
        std::source_location location;
        Level level;
    };

public: /* Methods */
    Logger(const time::Point& timeStart) : m_timeStart(timeStart)
    {
    }

    Logger() = delete;
    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    const std::deque<MsgData>& get_history() const
    {
        return m_msgHistory;
    }

    void set_level(Level level)
    {
        m_level = level;
    }

    LogProxy print_debug(std::source_location loc = std::source_location::current())
    {
        return {*this, Level::Debug, loc};
    }

    LogProxy print_info(std::source_location loc = std::source_location::current())
    {
        return {*this, Level::Info, loc};
    }

    LogProxy print_warning(std::source_location loc = std::source_location::current())
    {
        return {*this, Level::Warning, loc};
    }

    LogProxy print_error(std::source_location loc = std::source_location::current())
    {
        return {*this, Level::Error, loc};
    }

    static std::string_view level_to_string(Level level)
    {
        static_assert(std::to_underlying(Level::Count) == 6);

        switch (level)
        {
        case Level::All:
            return "All";

        case Level::Debug:
            return "Debug";

        case Level::Info:
            return "Info";

        case Level::Warning:
            return "Warning";

        case Level::Error:
            return "Error";

        case Level::None:
            return "None";

        case Level::Count:
            return "Count";

        default:
            return "Unknown";
        };
    }

private: /* Methods */
    template <typename... Args>
    void print_impl(Level level, std::source_location location, std::format_string<Args...> message, Args&&... args)
    {
        namespace chrono = std::chrono;

        static constexpr std::string_view colorReset = "\x1b[0m";
        static constexpr std::string_view colorPurple = "\x1b[0;1;35m";
        static constexpr std::string_view colorCyan = "\x1b[0;1;36m";
        static constexpr std::string_view colorYellow = "\x1b[0;1;33m";
        static constexpr std::string_view colorRed = "\x1b[0;1;31m";
        static constexpr std::string_view colorGray = "\x1b[0;1;30m";

        const time::Point timeNow = chrono::steady_clock::now();
        const chrono::hh_mm_ss<chrono::milliseconds> timeDisplay{
            chrono::duration_cast<chrono::milliseconds>(timeNow - m_timeStart)};

        const MsgData& msg =
            m_msgHistory.emplace_front(std::format(message, std::forward<Args>(args)...), timeDisplay, location, level);

        if (m_msgHistory.size() > HISTORY_MAX)
            m_msgHistory.pop_back();

        if (m_level > level)
            return;

        std::string_view funcName = clean_function_name(location.function_name());

        std::string_view colorText;

        switch (level)
        {
        case Level::Debug:
            colorText = colorPurple;
            break;

        case Level::Info:
        default:
            colorText = colorCyan;
            break;

        case Level::Warning:
            colorText = colorYellow;
            break;

        case Level::Error:
            colorText = colorRed;
            break;
        };

        const std::string levelLabel = std::format("[{}]", level_to_string(level));

        const std::string levelText = std::format("{:>10}", levelLabel);

        std::println("{}{} {}{}{} {:80} {}@ {} ({}:{}){}", colorGray, msg.time, colorText, levelText, colorReset,
                     msg.text, colorGray, funcName, location.file_name(), location.line(), colorReset);
    }

private: /* Fields */
    static constexpr std::size_t HISTORY_MAX = 5000;

    const time::Point& m_timeStart;

    std::deque<MsgData> m_msgHistory;

    Level m_level = Level::Info;
};
} // namespace rotm
