#include "game.hpp"

#include "core.hpp"
#include "exception.hpp"
#include "raylib.h"
#include "serialize_scene.hpp"
#include "tileset.hpp"

namespace rotm
{
Game::Game(Core &coreRef)
    : CoreModule(coreRef), m_player({0.0f, 0.0f}, core.resources.fetch_texture("resources/colon3.png"))
{
    auto result = read_scene("test map");

    if (!result.has_value())
    {
        throw exception::Base(result.error());
    }

    SceneFileData sceneData = std::move(*result);
    std::shared_ptr<TextureResource> tilesetAtlas = core.resources.fetch_texture(sceneData.tilesetPath);

    m_scene = std::make_unique<Scene>(Tileset{std::move(tilesetAtlas), sceneData.tilesetMetadata},
                                      sceneData.tilesetPath, std::move(sceneData.tilemap));
}

Game::~Game()
{
}

void Game::update()
{
    if (WindowShouldClose())
    {
        m_wantExit = true;
        return;
    }

    TilemapRenderer &tilemapRenderer = m_scene->render.tilemap;

    tilemapRenderer.update(m_scene->tileset, m_scene->tilemap, m_scene->camera);
}

void Game::draw()
{
    if (m_wantExit)
        return;

    TilemapRenderer &tilemapRenderer = m_scene->render.tilemap;
    const int tileSize = m_scene->tileset.get_tile_size();

    tilemapRenderer.draw(tileSize);

    m_player.draw();
}
} // namespace rotm
