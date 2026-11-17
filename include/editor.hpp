#pragma once

#include "core_module.hpp"
#include "logger.hpp"
#include "resource_manager.hpp"
#include "resource_texture.hpp"
#include "scene.hpp"
#include "serialize_scene.hpp"
#include "serialize_tileset.hpp"
#include "tilemap.hpp"

#include <array>
#include <expected>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include <imgui.h>
#include <raylib.h>

namespace rotm
{
class Editor final : public CoreModule
{
  public: /* Methods */
    Editor(Core &coreRef);
    ~Editor() override;

    void update() override;
    void tick() override;
    void draw() override;
    void sync_gui() override;

  private: /* Static properties */
    static constexpr char INI_PATH[] = "editor.ini";

    static constexpr char SCENE_NAME_DEFAULT[] = "Unnamed scene";

    static constexpr float SCENE_VIEW_CAM_ZOOM_MIN = 1.0f;
    static constexpr float SCENE_VIEW_CAM_ZOOM_MAX = 10.0f;

    static constexpr ImVec2 GUI_LOG_SIZE_DEFAULT = {640.0f, 480.0f};
    static constexpr float GUI_CONFIGURE_TILESET_TEX_SCALE = 4.0f;

    static constexpr Color BG_COLOR = {16, 16, 16, 255};

    static constexpr char TILESET_FILENAME_DEFAULT[] = "resources/tileset.png";

    static constexpr int TILE_SIZE_INPUT_DEFAULT = 2;
    static constexpr int TILE_SIZE_FACTOR = 8;

    static constexpr double TILE_CURSOR_ANIM_SPEED = 5.0;

    static constexpr int PENCIL_SIZE_MAX = TilemapChunk::SIZE;
    static constexpr int ERASER_SIZE_MAX = TilemapChunk::SIZE;

  private: /* Methods */
    void cfg_update();
    void cfg_apply();
    bool ini_load();
    void ini_save();

    Camera2D get_camera_default();
    Vector2 get_camera_offset();

    void init_scene(const TilesetMetadata &tilesetMetadata);
    void init_scene(const TilesetMetadata &tilesetMetadata, Tilemap &&tilemap);
    void destroy_scene();

    void begin_save_scene();
    void end_save_scene();
    bool is_save_in_progress() const;

    void begin_load_scene();
    void end_load_scene();
    bool is_load_in_progress() const;

    void begin_init_stress_test();
    void end_init_stress_test();
    bool is_init_stress_test_in_progress();

    void save_scene();
    // bool load_scene();

    bool load_tileset_atlas();

    void handle_shortcuts();
    void use_tool_at(const GlobalTileCoord &coord);
    void tool_pencil(const GlobalTileCoord &coord);
    void tool_eraser(const GlobalTileCoord &coord);

    void record_tile_edit(const GlobalTileCoord &position, const TileData &previous, const TileData &replacement);

    void undo();
    void redo();

    void draw_tile_cursor();
    void draw_edit_preview();
    void draw_origin_marker();

    void zoom_in_scene_view();
    void zoom_out_scene_view();

    void render_scene_view();

    void main_menu_bar();

    void window_welcome();

    void window_scene_editor();
    bool window_scene_editor_active();
    void window_scene_editor_handle_input(); // holy underscore batman
    void child_scene_editor_toolbox(const ImVec2 &size);
    void child_scene_editor_tilepalette(const ImVec2 &size);
    void child_scene_editor_tooloptions(const ImVec2 &size);

    void window_stats();

    void window_log();
    void window_log_menubar();
    void window_log_table();

    // void popup_new();
    void popup_enter_tileset_name();
    void popup_configure_tile_size();
    void popup_configure_tileset();

    void popup_open_scene();

    void popup_save_scene();

    void popup_init_stress_test();

  private: /* Fields */
    // Reference to core.logger
    Logger &logger;
    // Reference to core.resources
    ResourceManager &resources;

    // Session info to be saved to INI
    struct Config
    {
        struct Window
        {
            int x;
            int y;
            int width;
            int height;
            bool maximized;
        } window;
    } m_cfg;

    enum class EditTool : unsigned short
    {
        None = 0,
        Pencil,
        Eraser,
        Count
    } m_editTool = EditTool::Pencil;

    struct TileEdit
    {
        TileEdit() = delete;

        TileEdit(const GlobalTileCoord &_position, const TileData &_previous, const TileData &_replacement)
            : position(_position), previous(_previous), replacement(_replacement)
        {
        }

        GlobalTileCoord position;
        TileData previous;
        TileData replacement;
    };

    struct TileEditCommand
    {
        std::vector<TileEdit> edits;

        void apply(Tilemap &tilemap);
        void undo(Tilemap &tilemap);
        void redo(Tilemap &tilemap);
    };

    class EditHistory
    {
      public:
        void push(TileEditCommand command);
        void undo(Tilemap &tilemap);
        void redo(Tilemap &tilemap);
        void clear();

        bool can_undo() const
        {
            return m_cursor > 0;
        }
        bool can_redo() const
        {
            return m_cursor < m_commands.size();
        }

      private:
        std::vector<TileEditCommand> m_commands;
        std::size_t m_cursor = 0;
    };

    std::unique_ptr<Scene> m_scene;
    RenderTexture m_sceneView;

    std::optional<TileEditCommand> m_currentCommand;
    std::unordered_map<GlobalTileCoord, std::size_t> m_currentEditIndices;
    EditHistory m_editHistory;

    double m_tileCursorAnimCounter = 0.0;

    std::future<std::expected<SceneFileData, std::string>> m_loadJob;
    std::future<std::optional<std::string>> m_saveJob;

    std::future<Tilemap> m_stressTestJob;

    // GUI
    bool m_draggingCamera = false;

    std::string m_sceneName = SCENE_NAME_DEFAULT;
    std::string m_sceneNameInput = m_sceneName;
    bool m_sceneUnsavedChanges = false;
    bool m_sceneNeverSaved = false;
    bool m_sceneEditEnabled = false;

    bool m_openEnterTilesetName = false;
    bool m_openConfigureTileSize = false;
    bool m_openConfigureUsableTiles = false;
    bool m_openInitStressTest = false;

    bool m_openLoadScene = false;
    std::string m_sceneLoadErr;

    bool m_openSaveScene = false;
    std::string m_sceneSaveErr;

    std::string m_tilesetFilename = TILESET_FILENAME_DEFAULT;
    std::string m_tilesetFilenameInput = m_tilesetFilename;
    std::shared_ptr<TextureResource> m_tilesetAtlas;
    std::string m_tilesetLoadErr;
    int m_tileSizeInput = TILE_SIZE_INPUT_DEFAULT;
    std::vector<bool> m_usableTilesInput;

    struct GuiWindowSceneEditor
    {
        GlobalTileCoord mouseTile = {{0, 0}, {0, 0}};
        GlobalTileCoord mouseTilePrevious = mouseTile;
        TileData tileToPlace = {.id = 0};

        int pencilTileWidth = 1;
        int pencilTileHeight = 1;

        int eraserTileWidth = 1;
        int eraserTileHeight = 1;
    } m_windowSceneEditor;

    struct GuiChildTilePalette
    {
        bool selecting = false;
    } m_tilePalette;

    struct GuiWindowStats
    {
        bool active = false;
    } m_windowStats;

    struct GuiWindowLog
    {
        bool active = false;
        bool showColors = true;
        bool useLargeFont = false;
        std::array<bool, std::to_underlying(Logger::Level::Count)> showLevels = {
            false, // All (not a real log level)
            false, // Debug (ignored to avoid spam)
            true,  // Info
            true,  // Warning
            true,  // Error
            false  // None (not a real log level)
        };
    } m_windowLog;
};
} // namespace rotm
