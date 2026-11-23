#include "gui_context.hpp"

#include "core.hpp"

#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

namespace rotm
{
GuiContext::GuiContext(Core& coreRef) : core(coreRef), logger(core.logger)
{
    logger.print_info()("Initializing GUI...");

    rlImGuiSetup(USE_DARK_THEME);

    io = &ImGui::GetIO();

    io->IniFilename = nullptr; // disable ImGui persistence
    io->ConfigWindowsMoveFromTitleBarOnly = true;

    m_fonts = {.regular = io->Fonts->AddFontFromFileTTF("resources/iosevka.ttf", 21.5f),
               .regularSmall = io->Fonts->AddFontFromFileTTF("resources/iosevka.ttf", 16.0f),
               .monospace = io->Fonts->AddFontFromFileTTF("resources/iosevka_term_mono.ttf", 32.0f),
               .monospaceSmall = io->Fonts->AddFontFromFileTTF("resources/iosevka_term_mono.ttf", 20.0f)};
}

GuiContext::~GuiContext()
{
    logger.print_info()("Shutting down GUI...");

    rlImGuiShutdown();
}

void GuiContext::begin_frame()
{
    rlImGuiBeginDelta(static_cast<float>(core.get_delta_time()));
    ImGui::PushFont(m_fonts.regular);
}

void GuiContext::end_frame()
{
    ImGui::PopFont();
    rlImGuiEnd();
}

void GuiContext::window_log()
{
    if (!windowLog.active)
        return;

    ImGui::SetNextWindowSize(LOG_SIZE_DEFAULT, ImGuiCond_FirstUseEver);

    const bool visible = ImGui::Begin("Log", &windowLog.active, ImGuiWindowFlags_MenuBar);

    if (!visible)
        return ImGui::End();

    const GuiContext::Fonts& fonts = core.guiContext->get_fonts();

    window_log_menubar();

    ImGui::PushFont(windowLog.useLargeFont ? fonts.monospace : fonts.monospaceSmall);
    window_log_table();
    ImGui::PopFont();

    ImGui::End();
}

void GuiContext::window_log_menubar()
{
    using enum Logger::Level;
    using std::to_underlying;

    if (!ImGui::BeginMenuBar())
        return;

    if (ImGui::BeginMenu("View"))
    {
        // Flyout with log level toggles
        if (ImGui::BeginMenu("Log levels"))
        {
            ImGui::Checkbox("Debug", &windowLog.showLevels[to_underlying(Debug)]);
            ImGui::Checkbox("Info", &windowLog.showLevels[to_underlying(Info)]);
            ImGui::Checkbox("Warning", &windowLog.showLevels[to_underlying(Warning)]);
            ImGui::Checkbox("Error", &windowLog.showLevels[to_underlying(Error)]);
            ImGui::EndMenu();
        }

        ImGui::Checkbox("Show colors", &windowLog.showColors);
        ImGui::Checkbox("Use large font", &windowLog.useLargeFont);

        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

void GuiContext::window_log_table()
{
    static constexpr ImColor bgDebug = {0.3f, 0.1f, 0.3f, 1.0f};
    static constexpr ImColor bgDebugAlt = {0.35f, 0.15f, 0.35f, 1.0f};

    static constexpr ImColor bgInfo = {0.05f, 0.25f, 0.25f, 1.0f};
    static constexpr ImColor bgInfoAlt = {0.1f, 0.3f, 0.3f, 1.0f};

    static constexpr ImColor bgWarning = {0.3f, 0.3f, 0.1f, 1.0f};
    static constexpr ImColor bgWarningAlt = {0.35f, 0.35f, 0.15f, 1.0f};

    static constexpr ImColor bgError = {0.3f, 0.1f, 0.1f, 1.0f};
    static constexpr ImColor bgErrorAlt = {0.35f, 0.15f, 0.15f, 1.0f};

    const std::deque<Logger::MsgData>& logHistory = logger.get_history();
    const ImVec2 tableSize = ImGui::GetContentRegionAvail();

    const bool show = ImGui::BeginTable("##log-table", 1, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, tableSize);

    if (!show)
        return;

    // Was the scroll bar at the bottom last frame?
    const bool wasAtBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();

    const ImGuiStyle& style = ImGui::GetStyle();

    // Height of log message list
    float messagesHeight = 0.0f;

    for (const auto& message : logHistory)
    {
        if (!windowLog.showLevels[std::to_underlying(message.level)])
            continue;

        // const float textHeight = ImGui::CalcTextSize(message.text.c_str()).y;
        const float rowTextHeight = ImGui::GetTextLineHeight();

        messagesHeight += rowTextHeight + style.CellPadding.y * 2.0f;
    }

    // Fill unused space above short histories so the newest message sits at the bottom.
    const float topSpacerHeight = tableSize.y > messagesHeight ? tableSize.y - messagesHeight : 0.0f;

    if (topSpacerHeight > 0.0f)
    {
        ImGui::TableNextRow(0, topSpacerHeight);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4{0.0f, 0.0f, 0.0f, 0.0f}));
    }

    // Use primary or secondary table BG color?
    bool useAlternateBG = true;

    // Show log messages (iterated in reverse, queue front is newest message).
    for (auto it = logHistory.rbegin(); it != logHistory.rend(); ++it)
    {
        using enum Logger::Level;

        const Logger::MsgData& message = *it;

        // If we're ignoring this type of log message, skip this iteration.
        if (!windowLog.showLevels[std::to_underlying(message.level)])
            continue;

        ImGui::TableNextRow();

        if (windowLog.showColors)
        {
            // Alternate BG color per iteration.
            useAlternateBG = !useAlternateBG;

            // BG color for this row.
            ImColor rowColor;

            // Find BG color.
            switch (message.level)
            {
            case Debug:
                rowColor = useAlternateBG ? bgDebugAlt : bgDebug;
                break;

            case Warning:
                rowColor = useAlternateBG ? bgWarningAlt : bgWarning;
                break;

            case Error:
                rowColor = useAlternateBG ? bgErrorAlt : bgError;
                break;

            case Info:
            default:
                rowColor = useAlternateBG ? bgInfoAlt : bgInfo;
                break;
            }

            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowColor);
        }

        // Select the table's first column so the following text is submitted to its cell.
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(message.text.c_str());

        // Cleaned name of function that printed the message.
        const std::string_view funcName = clean_function_name(message.location.function_name());

        // Show tooltip with log metadata on hovering a message.
        ImGui::SetItemTooltip("Level: %s\n"
                              "Time: %02zi:%02zi:%02zi.%03zi\n"
                              "Function: %.*s\n"
                              "File: %s, line %u",
                              Logger::level_to_string(message.level).data(), message.time.hours().count(),
                              message.time.minutes().count(), message.time.seconds().count(),
                              message.time.subseconds().count(), static_cast<int>(funcName.size()), funcName.data(),
                              message.location.file_name(), message.location.line());
    }

    // If the scroll bar was at the bottom, autoscroll to the current bottom.
    if (wasAtBottom)
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndTable();
}
} // namespace rotm
