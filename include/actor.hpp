#pragma once

#include "resource_texture.hpp"
#include "sprite.hpp"
#include "tile_math.hpp"
#include "tilemap.hpp"
#include <memory>
#include <optional>
#include <raylib.h>

namespace rotm
{
/**
 * @brief Abstract class describing a dynamic object in a scene.
 *
 */
class Actor
{
  public: /* Methods */
    /**
     * @brief Construct a new Actor object at startPos.
     *
     * @param startPos Initial position in world space.
     */
    Actor(Vector2 startPos) : position(startPos)
    {
    }

    virtual ~Actor() = default;

    Actor() = delete;

    Actor(const Actor &) = delete;
    Actor &operator=(const Actor &) = delete;

    Actor(Actor &&) noexcept = default;
    Actor &operator=(Actor &&) noexcept = default;

    virtual void update(double deltaTime, const Tilemap &tilemap, int tileSize)
    {
        // suppress compiler warning
        (void)tilemap;
        (void)tileSize;

        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
    }

  public: /* Fields */
    // Position in world space.
    Vector2 position;
    // Movement speed in pixels/second.
    Vector2 velocity = {0.0f, 0.0f};
};

/**
 * @brief An Actor who is visualized by a sprite. (see Actor, Sprite)
 *
 */
class SpriteActor : public Actor
{
  public: /* Static properties */
    // How far to inset a given corner when checking collision against a given surface.
    static constexpr float CORNER_INSET = 0.5f;
    // How far to extend collision point when checking collision against a given surface.
    static constexpr float PROBE_DIST = 0.00001f;

  public: /* Methods */
    /**
     * @brief Construct a new Sprite Actor object at startPos, with sprite using spriteTexture.
     *
     * @param startPos Initial position in world space.
     * @param spriteTexture Texture for initializing sprite.
     */
    SpriteActor(Vector2 startPos, std::shared_ptr<TextureResource> &&spriteTexture)
        : Actor(startPos), sprite(std::move(spriteTexture))
    {
    }

    /**
     * @brief Trivial accessor for m_sprite.
     *
     * @return const Sprite& Reference to m_sprite.
     */
    virtual const Sprite &get_sprite() const
    {
        return sprite;
    }

    virtual void update(double deltaTime, const Tilemap &tilemap, int tileSize)
    {
        Actor::update(deltaTime, tilemap, tileSize);
    }

    virtual void draw() const
    {
        sprite.draw(position, scale);
    }

    virtual Rectangle get_rec() const
    {
        const Texture2D &texture = sprite.get_texture();
        const Vector2 textureSize = {static_cast<float>(texture.width), static_cast<float>(texture.height)};

        return {position.x, position.y, textureSize.x * scale.x, textureSize.y * scale.y};
    }

    virtual Vector2 center_point()
    {
        const Rectangle rec = get_rec();

        return {rec.x + (rec.width * 0.5f), rec.y + (rec.height * 0.5f)};
    }

    virtual Vector2 bottom_left_point() const
    {
        const Rectangle rec = get_rec();

        return {rec.x, rec.y + rec.height};
    }

    virtual Vector2 bottom_right_point() const
    {
        const Rectangle rec = get_rec();

        return {rec.x + rec.width, rec.y + rec.height};
    }

    virtual std::optional<GlobalTileCoord> touching_ground(const Tilemap &tilemap, int tileSize) const
    {
        const Vector2 leftPoint = bottom_left_point();
        const Vector2 rightPoint = bottom_right_point();

        const Vector2 leftTestPoint = {leftPoint.x + CORNER_INSET, leftPoint.y + PROBE_DIST};
        const Vector2 rightTestPoint = {rightPoint.x - CORNER_INSET, rightPoint.y + PROBE_DIST};

        const GlobalTileCoord leftTileCoord = point_to_tile(leftTestPoint, tileSize);
        const GlobalTileCoord rightTileCoord = point_to_tile(rightTestPoint, tileSize);

        const TileData *leftTile = tilemap.get_tile(leftTileCoord);
        const TileData *rightTile = tilemap.get_tile(rightTileCoord);

        if (leftTile and leftTile->id != TileData::ID_NULL)
            return leftTileCoord;

        if (rightTile and rightTile->id != TileData::ID_NULL)
            return rightTileCoord;

        return std::nullopt;
    }

  public:
    // Contains actor's graphical representation.
    Sprite sprite;
    // Multiplier of size relative to sprite texture.
    Vector2 scale = {1.0f, 1.0f};
};
} // namespace rotm
