#include "editor.hpp"
#include "core.hpp"
#include "exception.hpp"
#include "game.hpp"
#include "gui_context.hpp"
#include "logger.hpp"
#include "renderer_tilemap.hpp"
#include "scene.hpp"
#include "serialize_scene.hpp"
#include "serialize_tileset.hpp"
#include "tile_math.hpp"
#include "tilemap.hpp"

#include "utility.hpp"
#include "vec2i.hpp"
#include "version.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <deque>
#include <expected>
#include <filesystem>
#include <format>
#include <future>
#include <memory>
#include <numbers>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <raylib.h>
#include <rlImGui.h>
#include <tracy/Tracy.hpp>

#define MINI_CASE_SENSITIVE
#include <mini/ini.h>

namespace rotm
{
Editor::Editor(Core &coreRef) : CoreModule(coreRef), logger(core.logger), resources(core.resources)
{
    logger.print_info()("Initializing editor...");

    cfg_update();

    if (!ini_load())
        ini_save();

    cfg_apply();

    if (core.is_dev_mode())
    {
        using enum Logger::Level;
        using std::to_underlying;

        m_windowStats.active = true;

        m_windowLog.active = true;
        m_windowLog.showLevels[to_underlying(Debug)] = true;
    }
}

Editor::~Editor()
{
    logger.print_info()("Shutting down editor...");

    if (IsRenderTextureValid(m_sceneView))
        UnloadRenderTexture(m_sceneView);

    destroy_scene();

    ini_save();
}

void Editor::update()
{
    ZoneScopedN("Editor::update");

    if (WindowShouldClose())
    {
        if (m_sceneUnsavedChanges)
        {
            m_openUnsavedScene = true;
            m_unsavedSceneCallback = [this]() -> void { m_wantExit = true; };
        }
        else
        {
            m_wantExit = true;
            return;
        }
    }

    if (m_saveJob.valid() && !is_save_in_progress())
        end_save_scene();

    if (m_stressTestJob.valid() and !is_init_stress_test_in_progress())
        end_init_stress_test();

    cfg_update();

    handle_shortcuts();

    if (m_scene)
    {
        m_scene->render.tilemap.update(m_scene->tileset, m_scene->tilemap, m_scene->camera);
    }
}

void Editor::tick()
{
}

void Editor::draw()
{
    if (m_wantExit)
        return;

    ClearBackground(BG_COLOR);
}

void Editor::sync_gui()
{
    ZoneScopedN("Editor::sync_gui");

    // disable window resize grips
    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImColor{16, 16, 16}.Value);

    main_menu_bar();
    window_welcome();
    window_scene_editor();
    window_stats();
    window_log();

    // popup_new();
    popup_enter_tileset_name();
    popup_configure_tile_size();
    popup_configure_tileset();

    popup_open_scene();
    popup_save_scene();

    popup_init_stress_test();
    popup_unsaved_scene();

    ImGui::PopStyleColor(4);
}

void Editor::cfg_update()
{
    const Vector2 windowPos = GetWindowPosition();
    const bool windowMaximized = IsWindowMaximized();

    if (!windowMaximized)
    {
        m_cfg.window.x = static_cast<int>(windowPos.x);
        m_cfg.window.y = static_cast<int>(windowPos.y);
        m_cfg.window.width = GetScreenWidth();
        m_cfg.window.height = GetScreenHeight();
    }

    m_cfg.window.maximized = windowMaximized;
}

void Editor::cfg_apply()
{
    SetWindowPosition(m_cfg.window.x, m_cfg.window.y);
    SetWindowSize(m_cfg.window.width, m_cfg.window.height);

    if (m_cfg.window.maximized)
    {
        MaximizeWindow();
    }
    else
    {
        RestoreWindow();
    }
}

template <typename T>
void ini_write(mINI::INIStructure &ini, const std::string &section, const std::string &key, T value)
{
    ini[section][key] = std::to_string(value);
}

template <> void ini_write(mINI::INIStructure &ini, const std::string &section, const std::string &key, bool value)
{
    ini[section][key] = std::to_string(static_cast<int>(value));
}

void ini_read_int(Logger &logger, const mINI::INIStructure &ini, const std::string &section, const std::string &key,
                  int &target)
{
    if (!ini.has(section))
        return logger.print_warning()("INI missing section. (section={})", section);

    const mINI::INIMap map = ini.get(section);

    if (!map.has(key))
        return logger.print_warning()("INI missing key. (section={}, key={})", section, key);

    const std::string valueStr = map.get(key);

    try
    {
        target = std::stoi(valueStr);

        logger.print_info()("Loaded INI value. (section={}, key={}, value={})", section, key, target);
    }
    catch (const std::invalid_argument &)
    {
        logger.print_warning()("INI value is not an integer. (section={}, key={}, value={})", section, key, valueStr);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

bool Editor::ini_load()
{
    logger.print_info()("Loading INI config...");

    if (!std::filesystem::is_regular_file(INI_PATH))
    {
        logger.print_warning()("INI config file doesn't exist. (path={})", INI_PATH);
        return false;
    }

    mINI::INIFile file{INI_PATH};
    mINI::INIStructure ini;
    file.read(ini);

    auto readWindowInt = [&](const std::string &key, int &target) { ini_read_int(logger, ini, "Window", key, target); };

    readWindowInt("iPosX", m_cfg.window.x);
    readWindowInt("iPosY", m_cfg.window.y);
    readWindowInt("iSizeX", m_cfg.window.width);
    readWindowInt("iSizeY", m_cfg.window.height);
    readWindowInt("bMaximized", reinterpret_cast<int &>(m_cfg.window.maximized));

    return true;
}

#pragma GCC diagnostic pop

void Editor::ini_save()
{
    logger.print_info()("Saving INI config...");

    mINI::INIFile file{INI_PATH};

    mINI::INIStructure ini;

    auto writeWindowInt = [&ini](const std::string &key, int value) { ini_write(ini, "Window", key, value); };

    writeWindowInt("iPosX", m_cfg.window.x);
    writeWindowInt("iPosY", m_cfg.window.y);
    writeWindowInt("iSizeX", m_cfg.window.width);
    writeWindowInt("iSizeY", m_cfg.window.height);
    writeWindowInt("bMaximized", static_cast<int>(m_cfg.window.maximized));

    file.generate(ini, true);
}

Camera2D Editor::get_camera_default()
{
    return Camera2D{.offset = get_camera_offset(), .target = {0.0f, 0.0f}, .rotation = 0.0f, .zoom = 1.0f};
}

Vector2 Editor::get_camera_offset()
{
    return Vector2{.x = static_cast<float>(m_sceneView.texture.width) * 0.5f,
                   .y = static_cast<float>(m_sceneView.texture.height) * 0.5f};
}

void Editor::init_scene(const TilesetMetadata &tilesetMetadata)
{
    init_scene(tilesetMetadata, Tilemap{});
    m_sceneNeverSaved = true;
}

void Editor::init_scene(const TilesetMetadata &tilesetMetadata, Tilemap &&tilemap)
{
    m_scene = std::make_unique<Scene>(Tileset{std::move(m_tilesetAtlas), tilesetMetadata}, m_tilesetFilename,
                                      std::move(tilemap));

    m_scene->camera = get_camera_default();

    m_tilesetAtlas.reset();
    m_tileSizeInput = TILE_SIZE_INPUT_DEFAULT;
    m_usableTilesInput.clear();
    m_sceneUnsavedChanges = false;

    if (core.is_dev_mode())
    {
        m_scene->render.tilemap.showChunkBorders = true;
    }

    write_tileset_metadata(m_scene->tileset, m_tilesetFilename);

    window_stats_refresh_tile_count();
}

void Editor::destroy_scene()
{
    if (m_sceneUnsavedChanges)
    {
        m_openUnsavedScene = true;
        m_unsavedSceneCallback = [this]() -> void { destroy_scene(); };
        return;
    }

    m_scene.reset();
    m_editHistory.clear();
}

void Editor::begin_save_scene()
{
    if (!m_scene)
        return;

    if (m_saveJob.valid())
    {
        logger.print_warning()("Cannot save scene; a save is already in progress.");
        return;
    }

    m_saveJob = std::async(std::launch::async,
                           [&]() -> std::optional<std::string> { return write_scene(*m_scene, m_sceneName); });
}

void Editor::end_save_scene()
{
    std::optional<std::string> result = m_saveJob.get();

    if (result)
    {
        m_sceneSaveErr = std::move(*result);
        logger.print_warning()("{}", m_sceneSaveErr);
        return;
    }

    logger.print_info()("Saved scene. (m_sceneName={})", m_sceneName);
    m_sceneNeverSaved = false;
    m_sceneUnsavedChanges = false;
}

bool Editor::is_save_in_progress() const
{
    using namespace std::chrono_literals;

    return m_saveJob.valid() and m_saveJob.wait_for(0s) != std::future_status::ready;
}

void Editor::begin_load_scene()
{
    if (m_loadJob.valid())
    {
        logger.print_warning()("Cannot load scene; a load is already in progress.");
        return;
    }

    m_sceneLoadErr.clear();
    logger.print_info()("Loading scene... ({})", m_sceneName);

    m_loadJob = std::async(std::launch::async, read_scene, m_sceneName);
}

void Editor::end_load_scene()
{
    auto result = m_loadJob.get();

    if (!result.has_value())
    {
        m_sceneLoadErr = std::move(result.error());
        logger.print_warning()("{}", m_sceneLoadErr);
        return;
    }

    logger.print_info()("Done loading scene, initializing...");

    SceneFileData output = std::move(*result);

    m_tilesetFilename = output.tilesetPath.string();

    load_tileset_atlas();

    if (!m_tilesetAtlas)
    {
        logger.print_warning()("Failed to load tileset: {}", m_tilesetLoadErr);
        m_tilesetAtlas.reset();
        return;
    }

    init_scene(output.tilesetMetadata, std::move(output.tilemap));
}

bool Editor::is_load_in_progress() const
{
    using namespace std::chrono_literals;

    return m_loadJob.valid() and m_loadJob.wait_for(0s) != std::future_status::ready;
}

void Editor::begin_init_stress_test()
{
    if (is_init_stress_test_in_progress())
    {
        logger.print_warning()("Cannot initialize stress test; already in progress.");
        return;
    }

    logger.print_info()("Initializing stress test...");

    m_stressTestJob = std::async(std::launch::async, []() -> Tilemap {
        Tilemap out;
        out.stress_test();
        return out;
    });

    m_openInitStressTest = true;
}

void Editor::end_init_stress_test()
{
    m_scene->tilemap = m_stressTestJob.get();
    window_stats_refresh_tile_count();
}

bool Editor::is_init_stress_test_in_progress()
{
    using namespace std::chrono_literals;

    return m_stressTestJob.valid() and m_stressTestJob.wait_for(0s) != std::future_status::ready;
}

void Editor::save_scene()
{
    if (!m_scene)
    {
        logger.print_error()("Cannot save scene; no scene loaded.");
        return;
    }

    begin_save_scene();
}

void Editor::switch_to_game()
{
    try
    {
        core.request_module_transition(std::make_unique<Game>(core));
    }
    catch (const exception::Base &e)
    {
        logger.print_error()("Failed to switch to game module.");
    }
}

bool Editor::load_tileset_atlas()
{
    try
    {
        m_tilesetAtlas = resources.fetch_texture(m_tilesetFilename);

        return true;
    }
    catch (const exception::Base &e)
    {
        m_tilesetLoadErr = e.what();
    }

    return false;
}

void Editor::handle_shortcuts()
{
    const bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) or IsKeyDown(KEY_RIGHT_CONTROL);
    const bool shift = IsKeyDown(KEY_LEFT_SHIFT) or IsKeyDown(KEY_RIGHT_SHIFT);

    while (int keyPressed = GetKeyPressed())
    {
        if (keyPressed >= KEY_ZERO and keyPressed <= KEY_NINE)
        {
            using enum EditTool;
            const int number = keyPressed - KEY_ZERO;

            switch (static_cast<EditTool>(number))
            {
            case Pencil:
            case Brush:
            case Eraser:
                m_editTool = static_cast<EditTool>(number);
                break;

            default:
                break;
            }
        }
        else if (ctrl)
        {
            switch (keyPressed)
            {
            case KEY_NULL:
            default:
                break;

            case KEY_N:
                m_openEnterTilesetName = true;
                break;

            case KEY_O:
                m_openLoadScene = true;
                break;

            case KEY_S:
                m_openSaveScene = true;
                break;

            case KEY_W:
                destroy_scene();
                break;

            case KEY_Y:
                redo();
                break;

            case KEY_Z:
                undo();
                break;
            }
        }
        else if (shift)
        {
            switch (keyPressed)
            {
            case KEY_NULL:
            default:
                break;

            case KEY_GRAVE:
                begin_init_stress_test();
                break;
            }
        }
        else
        {
            switch (keyPressed)
            {
            case KEY_GRAVE:
                m_windowLog.active = !m_windowLog.active;
                break;

            case KEY_F3:
                m_windowStats.active = !m_windowStats.active;
                break;

            case KEY_F4: {
                TilemapRenderer &renderer = m_scene->render.tilemap;

                renderer.showChunkBorders = !renderer.showChunkBorders;

                break;
            }
            case KEY_HOME:
                if (m_sceneUnsavedChanges)
                {
                    m_openUnsavedScene = true;
                    m_unsavedSceneCallback = [this]() -> void { switch_to_game(); };
                }
                else
                {
                    switch_to_game();
                }

                break;
            }
        }

        keyPressed = GetKeyPressed();
    }
}

void Editor::use_tool_at(const GlobalTileCoord &coord)
{
    using enum EditTool;

    switch (m_editTool)
    {
    case None:
    default:
        break;

    case Pencil:
        tool_pencil(coord);
        break;

    case Eraser:
        tool_eraser(coord);
        break;
    }
}

void Editor::tool_pencil(const GlobalTileCoord &coord)
{
    const AbsoluteTileCoord coordAbs = global_to_absolute(coord);
    const int &width = m_windowSceneEditor.pencilTileWidth;
    const int &height = m_windowSceneEditor.pencilTileHeight;

    AbsoluteTileCoord tileMin = {coordAbs.x - (width / 2), coordAbs.y - (height / 2)};

    AbsoluteTileCoord tileMax = {coordAbs.x + (width / 2), coordAbs.y + (height / 2)};

    if (width % 2 == 0)
        ++tileMin.x;
    if (height % 2 == 0)
        ++tileMin.y;

    for (int y = tileMin.y; y <= tileMax.y; ++y)
    {
        for (int x = tileMin.x; x <= tileMax.x; ++x)
        {
            const GlobalTileCoord c = absolute_to_global({x, y});
            const TileData *prevPtr = m_scene->tilemap.get_tile(c);
            const TileData previous = prevPtr ? *prevPtr : TileData{};

            record_tile_edit(c, previous, m_windowSceneEditor.tileToPlace);
        }
    }
}

void Editor::tool_eraser(const GlobalTileCoord &coord)
{
    const TileData *prevPtr = m_scene->tilemap.get_tile(coord);

    if (!prevPtr)
        return;

    record_tile_edit(coord, *prevPtr, TileData{});
}

void Editor::record_tile_edit(const GlobalTileCoord &position, const TileData &previous, const TileData &replacement)
{
    const auto it = m_currentEditIndices.find(position);

    if (it != m_currentEditIndices.end())
    {
        m_currentCommand->edits[it->second].replacement = replacement;
        return;
    }

    const std::size_t idx = m_currentCommand->edits.size();

    m_currentCommand->edits.emplace_back(position, previous, replacement);

    m_currentEditIndices.emplace(position, idx);
}

void Editor::commit_tile_edit()
{
    m_currentCommand->apply(m_scene->tilemap);
    m_editHistory.push(std::move(*m_currentCommand));
    m_currentCommand.reset();
    m_currentEditIndices.clear();

    m_sceneUnsavedChanges = true;

    window_stats_refresh_tile_count();
}

void Editor::undo()
{
    m_editHistory.undo(m_scene->tilemap);
    m_sceneUnsavedChanges = true;

    window_stats_refresh_tile_count();
}

void Editor::redo()
{
    m_editHistory.redo(m_scene->tilemap);
    m_sceneUnsavedChanges = true;

    window_stats_refresh_tile_count();
}

void Editor::draw_tile_cursor()
{
    using std::numbers::pi;
    using enum EditTool;

    if (m_currentCommand)
        return;

    const AbsoluteTileCoord coordAbs = global_to_absolute(m_windowSceneEditor.mouseTile);
    const int tileSize = m_scene->tileset.get_tile_size();
    // const double sinNormalized = (std::sin(m_tileCursorAnimCounter * TILE_CURSOR_ANIM_SPEED) + 1) * 0.5;
    // m_tileCursorAnimCounter += core.get_delta_time();

    switch (m_editTool)
    {
    case Pencil: {
        const int &width = m_windowSceneEditor.pencilTileWidth;
        const int &height = m_windowSceneEditor.pencilTileHeight;

        AbsoluteTileCoord tileMin = {coordAbs.x - (width / 2), coordAbs.y - (height / 2)};
        AbsoluteTileCoord tileMax = {coordAbs.x + (width / 2), coordAbs.y + (height / 2)};

        if (width % 2 == 0)
            ++tileMin.x;
        if (height % 2 == 0)
            ++tileMin.y;

        std::vector<Rectangle> recs{static_cast<std::size_t>(width * height)};

        for (int y = tileMin.y; y <= tileMax.y; ++y)
        {
            for (int x = tileMin.x; x <= tileMax.x; ++x)
            {
                const GlobalTileCoord c = absolute_to_global({x, y});
                const Vector2 pos = tile_to_point(c, tileSize);

                Rectangle rec = {pos.x, pos.y, static_cast<float>(tileSize), static_cast<float>(tileSize)};

                recs.push_back(std::move(rec));
            }

            if (m_tileCursorAnimCounter >= pi * 2.0)
                m_tileCursorAnimCounter = 0;

            for (const Rectangle &rec : recs)
            {
                DrawRectangleLinesEx(rec, 2.0f, Fade(WHITE, 0.5f));
            }
        }
    }
    break;

    case Brush:
        break;

    case Eraser: {
        {
            const int &width = m_windowSceneEditor.eraserTileWidth;
            const int &height = m_windowSceneEditor.eraserTileHeight;

            AbsoluteTileCoord tileMin = {coordAbs.x - (width / 2), coordAbs.y - (height / 2)};
            AbsoluteTileCoord tileMax = {coordAbs.x + (width / 2), coordAbs.y + (height / 2)};

            if (width % 2 == 0)
                ++tileMin.x;
            if (height % 2 == 0)
                ++tileMin.y;

            std::vector<Rectangle> recs{static_cast<std::size_t>(width * height)};

            for (int y = tileMin.y; y <= tileMax.y; ++y)
            {
                for (int x = tileMin.x; x <= tileMax.x; ++x)
                {
                    const GlobalTileCoord c = absolute_to_global({x, y});
                    const Vector2 pos = tile_to_point(c, tileSize);

                    Rectangle rec = {pos.x, pos.y, static_cast<float>(tileSize), static_cast<float>(tileSize)};

                    recs.push_back(std::move(rec));
                }

                if (m_tileCursorAnimCounter >= pi * 2.0)
                    m_tileCursorAnimCounter = 0;

                for (const Rectangle &rec : recs)
                {
                    DrawRectangleLinesEx(rec, 2.0f, Fade(WHITE, 0.5f));
                }
            }
        }
    }
    break;

    default:
        break;
    }
}

void Editor::draw_edit_preview()
{
    using enum EditTool;

    if (!m_currentCommand)
        return;

    switch (m_editTool)
    {
    case Pencil:
        for (const TileEdit &edit : m_currentCommand->edits)
        {
            m_scene->render.tilemap.draw_tile(m_scene->tileset, edit.position, edit.replacement, Fade(WHITE, 0.33f));
        }
        break;

    case Eraser:
        for (const TileEdit &edit : m_currentCommand->edits)
        {
            const int tileSize = m_scene->tileset.get_tile_size();

            const Vector2 editPos = tile_to_point(edit.position, tileSize);

            const Rectangle rec = {.x = editPos.x,
                                   .y = editPos.y,
                                   .width = static_cast<float>(tileSize),
                                   .height = static_cast<float>(tileSize)};

            DrawRectangleRec(rec, Fade(BLACK, 0.66f));
        }
        break;

    default:
        break;
    }
}

void Editor::draw_origin_marker()
{
    static constexpr float size = 16.0f;
    static constexpr float halfSize = size * 0.5f;

    const Vector2 start1 = {.x = -halfSize, .y = 0.0f};

    const Vector2 end1 = {.x = halfSize, .y = 0.0f};

    const Vector2 start2 = {.x = 0.0f, .y = -halfSize};

    const Vector2 end2 = {.x = 0.0f, .y = halfSize};

    DrawLineV(start1, end1, WHITE);
    DrawLineV(start2, end2, WHITE);
}

void Editor::zoom_in_scene_view()
{
    m_scene->camera.zoom = std::min(m_scene->camera.zoom + 1.0f, SCENE_VIEW_CAM_ZOOM_MAX);
}

void Editor::zoom_out_scene_view()
{
    m_scene->camera.zoom = std::max(m_scene->camera.zoom - 1.0f, SCENE_VIEW_CAM_ZOOM_MIN);
}

void Editor::render_scene_view()
{
    if (!IsRenderTextureValid(m_sceneView))
        return;

    BeginTextureMode(m_sceneView);
    ClearBackground(BLACK);
    BeginMode2D(m_scene->camera);

    m_scene->render.tilemap.draw(m_scene->tileset.get_tile_size());

    draw_tile_cursor();
    draw_edit_preview();

    draw_origin_marker();

    EndMode2D();
    EndTextureMode();
}

void Editor::main_menu_bar()
{
    if (!ImGui::BeginMainMenuBar())
        return;

    if (ImGui::BeginMenu("File"))
    {
        const bool clickedNew = ImGui::MenuItem("New...", "CTRL + N");
        const bool clickedOpen = ImGui::MenuItem("Open...", "CTRL + O");
        const bool clickedSave = ImGui::MenuItem("Save", "CTRL + S");
        const bool clickedCloseScene = ImGui::MenuItem("Close scene", "CTRL + W", false, static_cast<bool>(m_scene));

        ImGui::Separator();
        const bool clickedExit = ImGui::MenuItem("Exit", "ALT + F4");

        if (clickedNew)
        {
            m_openEnterTilesetName = true;
        }
        else if (clickedOpen)
        {
            m_openLoadScene = true;
        }
        else if (clickedSave)
        {
            m_openSaveScene = true;
        }
        else if (clickedCloseScene)
        {
            destroy_scene();
        }
        else if (clickedExit)
        {
            if (m_sceneUnsavedChanges)
            {
                m_openUnsavedScene = true;
                m_unsavedSceneCallback = [this]() { m_wantExit = true; };
            }
            else
            {
                m_wantExit = true;
            }
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        const bool clickedUndo = ImGui::MenuItem(
            "Undo", "CTRL + Z", false, static_cast<bool>(m_scene) && static_cast<bool>(m_editHistory.can_undo()));
        const bool clickedRedo = ImGui::MenuItem(
            "Redo", "CTRL + Y", false, static_cast<bool>(m_scene) && static_cast<bool>(m_editHistory.can_redo()));

        if (clickedUndo)
        {
            undo();
        }
        else if (clickedRedo)
        {
            redo();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        const bool clickedStats = ImGui::MenuItem("Stats", "F3", m_windowStats.active);

        const bool clickedShowChunkBorders =
            ImGui::MenuItem("Show chunk borders", "F4", (m_scene ? m_scene->render.tilemap.showChunkBorders : false),
                            static_cast<bool>(m_scene));

        if (clickedStats)
        {
            m_windowStats.active = !m_windowStats.active;
        }

        if (clickedShowChunkBorders)
        {
            TilemapRenderer &renderer = m_scene->render.tilemap;

            renderer.showChunkBorders = !renderer.showChunkBorders;
        }

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void Editor::window_welcome()
{
    if (window_scene_editor_active())
        return;

    ImGui::SetNextWindowPos({static_cast<float>(GetScreenWidth()) * 0.5f, static_cast<float>(GetScreenHeight()) * 0.5f},
                            ImGuiCond_Always, {0.5f, 0.5f});

    const bool visible = ImGui::Begin(
        "Welcome", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    if (!visible)
        return ImGui::End();

    ImGui::Text("Welcome to the RotM v%i.%i scene editor!\n"
                "Select an option to get started.",
                version::MAJOR, version::MINOR);

    if (ImGui::Button("New..."))
    {
        m_openEnterTilesetName = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("Open..."))
    {
        m_openLoadScene = true;
    }

    ImGui::End();
}

void Editor::window_scene_editor()
{
    if (!window_scene_editor_active())
        return;

    static constexpr int styleVarCount = 1;
    static constexpr int styleColCount = 0;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {8.0f, 8.0f});

    const float screenW = static_cast<float>(GetScreenWidth());
    const float screenH = static_cast<float>(GetScreenHeight());

    ImGui::SetNextWindowSize({screenW * 0.8f, screenH * 0.8f}, ImGuiCond_FirstUseEver);

    ImGui::SetNextWindowSizeConstraints({320.0f, 240.0f}, {screenW * 0.9f, screenH * 0.9f});

    ImGui::SetNextWindowPos({screenW * 0.5f, screenH * 0.5f}, ImGuiCond_FirstUseEver, {0.5f, 0.5f});

    const bool visible =
        ImGui::Begin(std::format("Scene editor: {}", m_sceneName).c_str(), nullptr,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse |
                         (m_sceneUnsavedChanges ? ImGuiWindowFlags_UnsavedDocument : ImGuiWindowFlags_None));

    if (!visible)
    {
        ImGui::End();
        ImGui::PopStyleVar(styleVarCount);
        ImGui::PopStyleColor(styleColCount);
        return;
    }

    const ImVec2 available = ImGui::GetContentRegionAvail();

    const float toolboxWidth = available.x * 0.1f;

    child_scene_editor_toolbox(available);
    ImGui::SameLine();

    const ImVec2 rightPanelSize = {available.x - toolboxWidth - ImGui::GetStyle().ItemSpacing.x, available.y};

    const bool rightPanelVisible = ImGui::BeginChild("Scene Editor right panel", rightPanelSize);

    if (rightPanelVisible)
    {
        const ImVec2 rightPanelAvail = ImGui::GetContentRegionAvail();

        const int width = static_cast<int>(rightPanelAvail.x);
        const int height = static_cast<int>(rightPanelAvail.y * 0.75f);

        const bool reinitSceneView =
            ImGui::IsWindowAppearing() or width != m_sceneView.texture.width or height != m_sceneView.texture.height;

        if (reinitSceneView)
        {
            if (IsRenderTextureValid(m_sceneView))
                UnloadRenderTexture(m_sceneView);

            m_sceneView = LoadRenderTexture(width, height);
        }

        render_scene_view();

        m_scene->camera.offset = get_camera_offset();

        window_scene_editor_handle_input();

        ImGui::Image(static_cast<ImTextureID>(m_sceneView.texture.id),
                     {static_cast<float>(m_sceneView.texture.width), static_cast<float>(m_sceneView.texture.height)},
                     {0.0f, 1.0f}, {1.0f, 0.0f});

        m_sceneEditEnabled = ImGui::IsItemHovered() and !is_save_in_progress() and !is_init_stress_test_in_progress();

        const ImVec2 availSize = ImGui::GetContentRegionAvail();
        const ImVec2 paletteSize = {availSize.x * 0.5f, availSize.y};

        child_scene_editor_tilepalette(paletteSize);
        ImGui::SameLine();

        const ImVec2 availSize2 = ImGui::GetContentRegionAvail();
        const ImVec2 toolOptionsSize = availSize2;

        child_scene_editor_tooloptions(toolOptionsSize);
    }

    ImGui::EndChild();

    ImGui::End();

    ImGui::PopStyleVar(styleVarCount);
    ImGui::PopStyleColor(styleColCount);
}

bool Editor::window_scene_editor_active()
{
    return static_cast<bool>(m_scene);
}

void Editor::window_scene_editor_handle_input()
{
    const Camera2D &cam = m_scene->camera;

    const Vector2 mousePosScreen = GetMousePosition();

    const ImVec2 sceneViewPos = ImGui::GetCursorScreenPos();
    const Vector2 mousePosRelative = {((mousePosScreen.x - sceneViewPos.x - cam.offset.x) / cam.zoom) + cam.target.x,
                                      ((mousePosScreen.y - sceneViewPos.y - cam.offset.y) / cam.zoom) + cam.target.y};

    const Vector2 mouseDelta = GetMouseDelta();
    const Vector2 mouseDeltaScaled = {mouseDelta.x / m_scene->camera.zoom, mouseDelta.y / m_scene->camera.zoom};

    m_windowSceneEditor.mouseTilePrevious = m_windowSceneEditor.mouseTile;
    m_windowSceneEditor.mouseTile = point_to_tile(mousePosRelative, m_scene->tileset.get_tile_size());

    if (m_sceneEditEnabled)
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            m_currentCommand.emplace();
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
        {
            m_draggingCamera = true;
            SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
        }

        const float scroll = GetMouseWheelMove();

        if (scroll > 0.0f)
        {
            zoom_in_scene_view();
        }
        else if (scroll < 0.0f)
        {
            zoom_out_scene_view();
        }
    }

    if (m_currentCommand)
    {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            const AbsoluteTileCoord previous = global_to_absolute(m_windowSceneEditor.mouseTilePrevious);

            const AbsoluteTileCoord current = global_to_absolute(m_windowSceneEditor.mouseTile);

            for_each_tile_on_line(previous, current,
                                  [this](AbsoluteTileCoord coord) { use_tool_at(absolute_to_global(coord)); });
        }
        else
        {
            commit_tile_edit();
        }
    }

    if (m_draggingCamera)
    {
        if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
        {
            m_draggingCamera = false;
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }
        else
        {
            m_scene->camera.target.x -= mouseDeltaScaled.x;
            m_scene->camera.target.y -= mouseDeltaScaled.y;
        }
    }
}

void Editor::child_scene_editor_toolbox(const ImVec2 &windowMax)
{
    using enum EditTool;

    const float width = windowMax.x * 0.1f;
    const float height = windowMax.y;

    const bool visible = ImGui::BeginChild("Toolbox", {width, height});

    if (!visible)
        return ImGui::EndChild();

    const ImVec2 windowSize = ImGui::GetContentRegionAvail();

    const float buttonWidth = windowSize.x;

    ImGui::BeginDisabled(m_editTool == Pencil);
    const bool clickedPencil = ImGui::Button("  Pencil", {buttonWidth, 0.0f});
    ImGui::SetItemTooltip("Place tiles at mouse position. Configurable size.");
    ImGui::EndDisabled();

    ImGui::BeginDisabled(m_editTool == Brush);
    const bool clickedBrush = ImGui::Button("  Brush", {buttonWidth, 0.0f});
    ImGui::SetItemTooltip("Place tiles using prefabricated patterns.");
    ImGui::EndDisabled();

    ImGui::BeginDisabled(m_editTool == Eraser);
    const bool clickedEraser = ImGui::Button("  Eraser", {buttonWidth, 0.0f});
    ImGui::SetItemTooltip("Remove tiles at mouse position. Configurable size.");
    ImGui::EndDisabled();

    if (clickedPencil)
    {
        m_editTool = Pencil;
    }
    else if (clickedBrush)
    {
        m_editTool = Brush;
    }
    else if (clickedEraser)
    {
        m_editTool = Eraser;
    }

    ImGui::EndChild();
}

void Editor::child_scene_editor_tilepalette(const ImVec2 &size)
{
    const bool visible = ImGui::BeginChild("Tile Palette", size, ImGuiChildFlags_FrameStyle);

    if (!visible)
        return ImGui::EndChild();

    const Tileset &tileset = m_scene->tileset;
    const Texture2D &atlas = tileset.get_atlas();

    const float imageWidth = static_cast<float>(atlas.width);
    const float imageHeight = static_cast<float>(atlas.height);

    const float scale = std::min(std::floor(size.x / imageWidth), std::floor(size.y / imageHeight));

    const float scaleClamped = std::max(scale, 1.0f);

    const ImVec2 imageSizeScaled = {imageWidth * scaleClamped, imageHeight * scaleClamped};

    const ImVec2 imagePos = ImGui::GetCursorScreenPos();

    ImGui::Image(static_cast<ImTextureID>(atlas.id), imageSizeScaled);

    ImDrawList *drawList = ImGui::GetWindowDrawList();

    const int tileSize = tileset.get_tile_size();
    const float tileSizeScaled = tileSize * scaleClamped;
    const int atlasTileWidth = atlas.width / tileSize;
    const int atlasTileHeight = atlas.height / tileSize;

    const Vector2 mousePos = GetMousePosition();
    const Vector2 mousePosRelative = {mousePos.x - imagePos.x, mousePos.y - imagePos.y};

    const Vec2I tileHovering = {static_cast<int>(floor(mousePosRelative.x / tileSizeScaled)),

                                static_cast<int>(floor(mousePosRelative.y / tileSizeScaled))};

    if (tileHovering.x < atlasTileWidth and tileHovering.y < atlasTileHeight)
    {
        const TileData::IDType hoveredID = tileHovering.y * atlasTileWidth + tileHovering.x;

        if (tileset.has_tile(hoveredID))
        {
            if (m_windowSceneEditor.tileToPlace.id != hoveredID)
            {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                {
                    m_windowSceneEditor.tileToPlace.id = hoveredID;
                }
                else
                {
                    const ImVec2 hoveredMin = {static_cast<float>(tileHovering.x) * tileSizeScaled + imagePos.x,
                                               static_cast<float>(tileHovering.y) * tileSizeScaled + imagePos.y};

                    const ImVec2 hoveredMax = {hoveredMin.x + tileSizeScaled, hoveredMin.y + tileSizeScaled};

                    drawList->AddRect(hoveredMin, hoveredMax, ImColor{1.0f, 1.0f, 1.0f, 0.5f});
                }
            }
        }
    }

    const Vec2I tileSelected = {m_windowSceneEditor.tileToPlace.id % atlasTileWidth,
                                m_windowSceneEditor.tileToPlace.id / atlasTileWidth};

    const ImVec2 selectedMin = {static_cast<float>(tileSelected.x) * tileSizeScaled + imagePos.x,
                                static_cast<float>(tileSelected.y) * tileSizeScaled + imagePos.y};

    const ImVec2 selectedMax = {selectedMin.x + tileSizeScaled, selectedMin.y + tileSizeScaled};

    drawList->AddRectFilled(selectedMin, selectedMax, ImColor{1.0f, 1.0f, 1.0f, 0.5f});

    ImGui::EndChild();
}

void Editor::child_scene_editor_tooloptions(const ImVec2 &size)
{
    using enum EditTool;

    const bool visible = ImGui::BeginChild("Tool options", size);

    if (!visible)
        return ImGui::EndChild();

    switch (m_editTool)
    {
    case Pencil:
        ImGui::SliderInt("Pencil width", &m_windowSceneEditor.pencilTileWidth, 1, PENCIL_SIZE_MAX);
        ImGui::SliderInt("Pencil height", &m_windowSceneEditor.pencilTileHeight, 1, PENCIL_SIZE_MAX);
        break;

    case Brush:
        child_scene_editor_tooloptions_brusheditor();
        break;

    case Eraser:
        ImGui::SliderInt("Eraser width", &m_windowSceneEditor.eraserTileWidth, 1, ERASER_SIZE_MAX);
        ImGui::SliderInt("Eraser height", &m_windowSceneEditor.eraserTileHeight, 1, ERASER_SIZE_MAX);
        break;

    default:
        ImGui::TextUnformatted("Invalid tool selected! Izzy do something!!!");
        break;
    }

    ImGui::EndChild();
}

void Editor::child_scene_editor_tooloptions_brusheditor()
{
    ImGui::TextUnformatted("THIS SHIT DON'T WORK!!!!");
}

void Editor::window_stats()
{
    if (!m_windowStats.active)
        return;

    ImGui::SetNextWindowPos({0.0f, static_cast<float>(GetScreenHeight())}, ImGuiCond_Always, {0.0f, 1.0f});

    const bool visible = ImGui::Begin("Stats", &m_windowStats.active,
                                      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                                          ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse |
                                          ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing);

    if (!visible)
        return ImGui::End();

    ImGui::Text("%i FPS", core.get_fps());

    if (m_scene)
    {
        const TilemapRenderer &tilemapRenderer = m_scene->render.tilemap;

        ImGui::SeparatorText("Tilemap");

        ImGui::Text("%zu tiles", m_windowStats.tileCountLastEdit);
        ImGui::Text("%zu chunks", m_windowStats.chunkCountLastEdit);

        ImGui::SeparatorText("Render");

        ImGui::Text("%zu visible chunks", tilemapRenderer.get_visible_chunk_count());
        ImGui::Text("%zu chunks drawn", tilemapRenderer.get_chunks_drawn_count());
    }

    ImGui::End();
}

void Editor::window_stats_refresh_tile_count()
{
    m_windowStats.tileCountLastEdit = m_scene->tilemap.get_tile_count();
    m_windowStats.chunkCountLastEdit = m_scene->tilemap.get_chunk_count();
}

void Editor::window_log()
{
    if (!m_windowLog.active)
        return;

    ImGui::SetNextWindowSize(GUI_LOG_SIZE_DEFAULT, ImGuiCond_FirstUseEver);

    const bool visible = ImGui::Begin("Log", &m_windowLog.active, ImGuiWindowFlags_MenuBar);

    if (!visible)
        return ImGui::End();

    const GuiContext &guiContext = core.get_gui_context();
    const GuiContext::Fonts &fonts = guiContext.get_fonts();

    window_log_menubar();

    ImGui::PushFont(m_windowLog.useLargeFont ? fonts.monospace : fonts.monospaceSmall);
    window_log_table();
    ImGui::PopFont();

    ImGui::End();
}

void Editor::window_log_menubar()
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
            ImGui::Checkbox("Debug", &m_windowLog.showLevels[to_underlying(Debug)]);
            ImGui::Checkbox("Info", &m_windowLog.showLevels[to_underlying(Info)]);
            ImGui::Checkbox("Warning", &m_windowLog.showLevels[to_underlying(Warning)]);
            ImGui::Checkbox("Error", &m_windowLog.showLevels[to_underlying(Error)]);
            ImGui::EndMenu();
        }

        ImGui::Checkbox("Show colors", &m_windowLog.showColors);
        ImGui::Checkbox("Use large font", &m_windowLog.useLargeFont);

        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

void Editor::window_log_table()
{
    static constexpr ImColor bgDebug = {0.3f, 0.1f, 0.3f, 1.0f};
    static constexpr ImColor bgDebugAlt = {0.35f, 0.15f, 0.35f, 1.0f};

    static constexpr ImColor bgInfo = {0.05f, 0.25f, 0.25f, 1.0f};
    static constexpr ImColor bgInfoAlt = {0.1f, 0.3f, 0.3f, 1.0f};

    static constexpr ImColor bgWarning = {0.3f, 0.3f, 0.1f, 1.0f};
    static constexpr ImColor bgWarningAlt = {0.35f, 0.35f, 0.15f, 1.0f};

    static constexpr ImColor bgError = {0.3f, 0.1f, 0.1f, 1.0f};
    static constexpr ImColor bgErrorAlt = {0.35f, 0.15f, 0.15f, 1.0f};

    const std::deque<Logger::MsgData> &logHistory = logger.get_history();
    const ImVec2 tableSize = ImGui::GetContentRegionAvail();

    const bool show = ImGui::BeginTable("##log-table", 1, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, tableSize);

    if (!show)
        return;

    // Was the scroll bar at the bottom last frame?
    const bool wasAtBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();

    const ImGuiStyle &style = ImGui::GetStyle();

    // Height of log message list
    float messagesHeight = 0.0f;

    for (const auto &message : logHistory)
    {
        if (!m_windowLog.showLevels[std::to_underlying(message.level)])
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

        const Logger::MsgData &message = *it;

        // If we're ignoring this type of log message, skip this iteration.
        if (!m_windowLog.showLevels[std::to_underlying(message.level)])
            continue;

        ImGui::TableNextRow();

        if (m_windowLog.showColors)
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

void Editor::popup_enter_tileset_name()
{
    if (m_openEnterTilesetName)
    {
        ImGui::OpenPopup("Enter tileset name");
        m_openEnterTilesetName = false;
    }

    const bool active = ImGui::BeginPopupModal("Enter tileset name", nullptr,
                                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

    if (!active)
        return;

    ImGui::TextUnformatted("Enter the name of an image file to load as tileset atlas:");

    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();
    const bool hitEnter = ImGui::InputText("##tileset-name", &m_tilesetFilenameInput,
                                           ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

    ImGui::SameLine();

    if (ImGui::Button("OK") or hitEnter)
    {
        m_tilesetFilename = m_tilesetFilenameInput;

        if (!load_tileset_atlas())
        {
            logger.print_warning()("{}", m_tilesetLoadErr);
            ImGui::OpenPopup("Invalid tileset name");
        }
        else
        {
            const auto metadataResult = read_tileset_metadata(m_tilesetFilename);

            if (!metadataResult.has_value())
            {
                logger.print_warning()("{}", metadataResult.error());

                m_openConfigureTileSize = true;
                ImGui::CloseCurrentPopup();
            }
            else
            {
                logger.print_info()("Got tileset metadata. ({})", m_tilesetFilename);
                init_scene(metadataResult.value());

                ImGui::CloseCurrentPopup();
            }
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        m_tilesetFilenameInput = m_tilesetFilename;
        ImGui::CloseCurrentPopup();
    }

    const bool errorActive = ImGui::BeginPopupModal("Invalid tileset name", nullptr,
                                                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

    if (errorActive)
    {
        ImGui::Text("%s", m_tilesetLoadErr.c_str());

        if (ImGui::Button("OK"))
        {
            m_tilesetLoadErr.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::EndPopup();
}

void Editor::popup_configure_tile_size()
{
    if (m_openConfigureTileSize)
    {
        ImGui::OpenPopup("Configure tile size");
        m_openConfigureTileSize = false;
    }

    const bool active = ImGui::BeginPopupModal("Configure tile size", nullptr,
                                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

    if (!active)
        return;

    const Texture2D &atlasTexture = m_tilesetAtlas->get();

    const ImVec2 textureSizeScaled = {static_cast<float>(atlasTexture.width) * GUI_CONFIGURE_TILESET_TEX_SCALE,
                                      static_cast<float>(atlasTexture.height) * GUI_CONFIGURE_TILESET_TEX_SCALE};

    ImGui::TextUnformatted("Set the size of a tile.");

    const ImVec2 imagePos = ImGui::GetCursorScreenPos();

    ImGui::Image(static_cast<ImTextureID>(atlasTexture.id), textureSizeScaled);

    ImDrawList *drawList = ImGui::GetWindowDrawList();

    const int actualTileSize = m_tileSizeInput * TILE_SIZE_FACTOR;

    const int atlasTileWidth = atlasTexture.width / actualTileSize;
    const int atlasTileHeight = atlasTexture.height / actualTileSize;

    const float rectSide = static_cast<float>(actualTileSize) * GUI_CONFIGURE_TILESET_TEX_SCALE;

    const ImVec2 rectSize = {rectSide, rectSide};

    for (int y = 0; y < atlasTileHeight; ++y)
    {
        for (int x = 0; x < atlasTileWidth; ++x)
        {
            const ImVec2 rectPos = {(static_cast<float>(x) * rectSize.x) + imagePos.x,
                                    (static_cast<float>(y) * rectSize.y) + imagePos.y};

            drawList->AddRect(rectPos, {rectPos.x + rectSize.x, rectPos.y + rectSize.y}, ImColor{1.0f, 1.0f, 1.0f});
        }
    }

    const std::string sliderFmt = std::format("{}x{}", actualTileSize, actualTileSize);

    ImGui::SliderInt("##tile-size-slider", &m_tileSizeInput, 1, TILE_SIZE_FACTOR, sliderFmt.c_str());

    ImGui::SameLine();

    if (ImGui::Button("OK"))
    {
        const std::size_t atlasTileCount =
            static_cast<std::size_t>(atlasTileWidth) * static_cast<std::size_t>(atlasTileHeight);

        m_usableTilesInput.resize(atlasTileCount, true);

        m_openConfigureUsableTiles = true;
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        m_tileSizeInput = TILE_SIZE_INPUT_DEFAULT;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void Editor::popup_configure_tileset()
{
    if (m_openConfigureUsableTiles)
    {
        ImGui::OpenPopup("Configure tileset");
        m_openConfigureUsableTiles = false;
    }

    const bool active = ImGui::BeginPopupModal("Configure tileset", nullptr,
                                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

    if (!active)
        return;

    const Texture2D &atlasTexture = m_tilesetAtlas->get();

    const ImVec2 textureSizeScaled = {static_cast<float>(atlasTexture.width) * GUI_CONFIGURE_TILESET_TEX_SCALE,
                                      static_cast<float>(atlasTexture.height) * GUI_CONFIGURE_TILESET_TEX_SCALE};

    ImGui::TextUnformatted("Select all usable tiles, and deselect blank space or non-tile graphics.");

    const ImVec2 imagePos = ImGui::GetCursorScreenPos();

    ImGui::Image(static_cast<ImTextureID>(atlasTexture.id), textureSizeScaled);

    const bool imageHovered = ImGui::IsItemHovered();

    ImDrawList *drawList = ImGui::GetWindowDrawList();

    const int actualTileSize = m_tileSizeInput * TILE_SIZE_FACTOR;

    const float rectSide = static_cast<float>(actualTileSize) * GUI_CONFIGURE_TILESET_TEX_SCALE;
    const ImVec2 rectSize = {rectSide, rectSide};
    const int atlasTileWidth = atlasTexture.width / actualTileSize;

    std::optional<std::size_t> hoveredTileIdx = std::nullopt;

    if (imageHovered)
    {
        const Vector2 mousePos = GetMousePosition();

        const Vector2 mousePosRelative = {.x = mousePos.x - imagePos.x, .y = mousePos.y - imagePos.y};

        const std::size_t tileX = static_cast<std::size_t>(mousePosRelative.x / rectSize.x);

        const std::size_t tileY = static_cast<std::size_t>(mousePosRelative.y / rectSize.y);

        hoveredTileIdx = (tileY * atlasTileWidth) + tileX;
    }

    if (hoveredTileIdx and IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        m_usableTilesInput[hoveredTileIdx.value()] = !m_usableTilesInput[hoveredTileIdx.value()];
    }

    for (std::size_t i = 0; i < m_usableTilesInput.size(); ++i)
    {
        if (!m_usableTilesInput[i])
            continue;

        const std::size_t x = i % atlasTileWidth;
        const std::size_t y = i / atlasTileWidth;

        const ImVec2 rectPos = {(static_cast<float>(x) * rectSize.x) + imagePos.x,
                                (static_cast<float>(y) * rectSize.y) + imagePos.y};

        drawList->AddRect(rectPos, {rectPos.x + rectSize.x, rectPos.y + rectSize.y}, ImColor{1.0f, 1.0f, 1.0f});
    }

    if (ImGui::Button("OK"))
    {
        const TilesetMetadata meta = {
            .tileSize = actualTileSize,
            .tileCount = m_usableTilesInput.size(),
            .usableTilesPacked = pack_bitset(m_usableTilesInput) // inefficient but i really can't be bothered
        };

        init_scene(meta);

        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        m_tilesetAtlas.reset();
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void Editor::popup_open_scene()
{
    if (m_openLoadScene)
    {
        ImGui::OpenPopup("Open scene");
        m_openLoadScene = false;
    }

    ImGui::SetNextWindowPos({static_cast<float>(GetScreenWidth()) * 0.5f, static_cast<float>(GetScreenHeight()) * 0.5f},
                            ImGuiCond_Always, {0.5f, 0.5f});

    const bool active =
        ImGui::BeginPopupModal("Open scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

    if (!active)
        return;

    if (m_loadJob.valid() && !is_load_in_progress())
    {
        end_load_scene();

        if (!m_sceneLoadErr.empty())
            ImGui::OpenPopup("Invalid scene");
        else
            ImGui::CloseCurrentPopup();
    }

    if (is_load_in_progress())
    {
        ImGui::TextUnformatted("Loading scene...");
    }
    else
    {
        ImGui::TextUnformatted("Enter the name of the scene you want to open (without file extension).");

        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        const bool hitEnter =
            ImGui::InputText("##scene-name", &m_sceneNameInput,
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

        ImGui::SameLine();

        if (ImGui::Button("OK") or hitEnter)
        {
            m_sceneName = m_sceneNameInput;
            begin_load_scene();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            m_sceneNameInput = m_sceneName;
            ImGui::CloseCurrentPopup();
        }

        popup_open_scene_table();
    }

    const bool errorActive =
        ImGui::BeginPopupModal("Invalid scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

    if (errorActive)
    {
        ImGui::Text("%s", m_sceneLoadErr.c_str());

        if (ImGui::Button("OK"))
        {
            m_sceneLoadErr.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::EndPopup();
}

void Editor::popup_open_scene_table()
{
    namespace fs = std::filesystem;

    const bool show = ImGui::BeginTable("##loadable-scenes", 1, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY);

    if (show)
    {
        if (m_loadableScenes.empty())
        {
            const fs::directory_iterator sceneRoot{SCENE_ROOT};

            for (const fs::path &sceneFolder : sceneRoot)
            {
                if (!fs::is_directory(sceneFolder))
                    continue;

                if (!fs::is_regular_file(sceneFolder / "tilemap.rtm"))
                    continue;

                m_loadableScenes.push_back(sceneFolder);
            }
        }
        else
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted("Available scenes:");

            for (const fs::path &scene : m_loadableScenes)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(scene.string().c_str());

                if (ImGui::IsItemHovered() and IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                {
                    m_sceneNameInput = scene.stem().string();
                    m_sceneName = m_sceneNameInput;
                    begin_load_scene();
                }
            }
        }

        ImGui::EndTable();
    }
}

void Editor::popup_save_scene()
{
    if (m_openSaveScene)
    {
        ImGui::OpenPopup("Save scene");
        m_openSaveScene = false;
    }

    ImGui::SetNextWindowPos({static_cast<float>(GetScreenWidth()) * 0.5f, static_cast<float>(GetScreenHeight()) * 0.5f},
                            ImGuiCond_Always, {0.5f, 0.5f});

    const bool active =
        ImGui::BeginPopupModal("Save scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

    if (!active)
        return;

    if (m_saveJob.valid() and !is_save_in_progress())
    {
        end_save_scene();

        if (!m_sceneSaveErr.empty())
            ImGui::OpenPopup("Save failed");
        else
            ImGui::CloseCurrentPopup();
    }

    if (is_save_in_progress())
    {
        ImGui::TextUnformatted("Saving scene...");
    }
    else if (m_sceneNeverSaved)
    {
        ImGui::TextUnformatted("Enter the name of the scene to be saved (no file extension):");

        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        const bool hitEnter =
            ImGui::InputText("##scene-name", &m_sceneNameInput,
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

        if (ImGui::Button("OK") or hitEnter)
        {
            m_sceneName = m_sceneNameInput;
            save_scene();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            m_sceneNameInput = m_sceneName;
            ImGui::CloseCurrentPopup();
        }
    }
    else if (ImGui::IsWindowAppearing())
        save_scene();
    else if (!is_save_in_progress())
        ImGui::CloseCurrentPopup();

    const bool errorActive =
        ImGui::BeginPopupModal("Save failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

    if (errorActive)
    {
        ImGui::Text("%s", m_sceneLoadErr.c_str());

        if (ImGui::Button("OK"))
        {
            m_sceneLoadErr.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::EndPopup();
}

void Editor::popup_init_stress_test()
{
    if (m_openInitStressTest)
    {
        ImGui::OpenPopup("Initializing stress test");
        m_openInitStressTest = false;
    }

    const bool active = ImGui::BeginPopupModal("Initializing stress test", nullptr,
                                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

    if (!active)
        return;

    ImGui::TextUnformatted("Beginning stress test...");

    if (!is_init_stress_test_in_progress())
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}

void Editor::popup_unsaved_scene()
{
    if (m_openUnsavedScene)
    {
        ImGui::OpenPopup("Unsaved scene edits");
        m_openUnsavedScene = false;
    }

    const bool active = ImGui::BeginPopupModal("Unsaved scene edits", nullptr,
                                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

    if (!active)
        return;

    if (m_unsavedSceneBeganSave)
    {
        if (is_save_in_progress())
        {
            ImGui::TextUnformatted("Saving...");
        }
        else if (m_sceneSaveErr.empty())
        {
            ImGui::CloseCurrentPopup();
            m_unsavedSceneBeganSave = false;
            m_unsavedSceneCallback();
            m_unsavedSceneCallback = nullptr;
        }
        else
        {
            ImGui::OpenPopup("Save failed");
        }
    }
    else
    {
        ImGui::TextUnformatted("This scene has unsaved changes. Do you want to save?");

        const bool pressedOK = ImGui::Button("Save");
        ImGui::SameLine();
        const bool pressedDontSave = ImGui::Button("Don't save");
        ImGui::SameLine();
        const bool pressedCancel = ImGui::Button("Cancel");

        if (pressedOK)
        {
            m_unsavedSceneBeganSave = true;
            begin_save_scene();
        }
        else if (pressedDontSave)
        {
            ImGui::CloseCurrentPopup();
            m_sceneUnsavedChanges = false;
            m_unsavedSceneCallback();
            m_unsavedSceneCallback = nullptr;
        }
        else if (pressedCancel)
        {
            ImGui::CloseCurrentPopup();
        }
    }

    const bool errorActive =
        ImGui::BeginPopupModal("Save failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

    if (errorActive)
    {
        ImGui::Text("%s", m_sceneLoadErr.c_str());

        if (ImGui::Button("OK"))
        {
            m_sceneLoadErr.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::EndPopup();
}

void Editor::TileEditCommand::apply(Tilemap &tilemap)
{
    for (const TileEdit &edit : edits)
    {
        tilemap.set_tile(edit.position, edit.replacement);
    }
}

void Editor::TileEditCommand::undo(Tilemap &tilemap)
{
    for (const TileEdit &edit : edits)
    {
        tilemap.set_tile(edit.position, edit.previous);
    }
}

void Editor::TileEditCommand::redo(Tilemap &tilemap)
{
    apply(tilemap);
}

void Editor::EditHistory::push(TileEditCommand command)
{
    using std::size_t;

    m_commands.erase(m_commands.begin() + m_cursor, m_commands.end());

    m_commands.emplace_back(std::move(command));
    ++m_cursor;
}

void Editor::EditHistory::undo(Tilemap &tilemap)
{
    if (!can_undo())
        return;

    --m_cursor;
    m_commands.at(m_cursor).undo(tilemap);
}

void Editor::EditHistory::redo(Tilemap &tilemap)
{
    if (!can_redo())
        return;

    m_commands.at(m_cursor).redo(tilemap);
    ++m_cursor;
}

void Editor::EditHistory::clear()
{
    m_commands.clear();
    m_cursor = 0;
}
} // namespace rotm
