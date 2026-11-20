#include "game.hpp"

#include "core.hpp"
#include "editor.hpp"
#include "exception.hpp"
#include "raylib.h"
#include "serialize_scene.hpp"
#include "tileset.hpp"

namespace rotm
{
Game::Game(Core &coreRef)
    : CoreModule(coreRef), m_player({0.0f, 0.0f}, core.resources.fetch_texture("resources/colon3.png"))
{
    auto result = read_scene("test scene");

    if (!result.has_value())
    {
        throw exception::Base(std::format("Failed to load scene. ({})", result.error()));
    }

    SceneFileData sceneData = std::move(*result);
    std::shared_ptr<TextureResource> tilesetAtlas = core.resources.fetch_texture(sceneData.tilesetPath);

    m_scene = std::make_unique<Scene>(Tileset{std::move(tilesetAtlas), sceneData.tilesetMetadata},
                                      sceneData.tilesetPath, std::move(sceneData.tilemap));

    Camera2D &cam = m_scene->camera;

    cam.rotation = 0.0f;
    cam.zoom = 4.0f;

    update_camera();
}

Game::~Game()
{
}

void Game::update()
{
    if (!m_scene)
        throw exception::Base("Game: Missing scene!");

    Tilemap &tilemap = m_scene->tilemap;
    const Tileset &tileset = m_scene->tileset;

    const double deltaTime = core.get_delta_time();

    if (WindowShouldClose())
    {
        m_wantExit = true;
        return;
    }

    if (IsKeyPressed(KEY_HOME))
        core.request_module_transition(std::make_unique<Editor>(core));

    m_player.update(deltaTime, tilemap, tileset.get_tile_size());

    update_camera();

    m_scene->render.tilemap.update(tileset, tilemap, m_scene->camera);
}

void Game::draw()
{
    if (m_wantExit)
        return;

    BeginMode2D(m_scene->camera);

    TilemapRenderer &tilemapRenderer = m_scene->render.tilemap;
    const int tileSize = m_scene->tileset.get_tile_size();

    tilemapRenderer.draw(tileSize);

    m_player.draw();

    EndMode2D();

    DrawText(TextFormat("%i", core.get_fps()), 10, 10, 20, Fade(WHITE, 0.25f));
}

void Game::update_camera()
{
    m_scene->camera.offset = get_camera_offset();
    m_scene->camera.target = m_player.center_point(m_player.position);
}

Vector2 Game::get_camera_offset()
{
    return {static_cast<float>(GetScreenWidth()) * 0.5f, static_cast<float>(GetScreenHeight()) * 0.5f};
}
} // namespace rotm
