#pragma once

#include "resource_texture.hpp"
#include "sprite.hpp"
#include "tile_math.hpp"
#include "tilemap.hpp"

// #include <cmath>
#include <memory>
#include <optional>

#include <raylib.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <raymath.h>
#pragma GCC diagnostic pop
#include <tracy/Tracy.hpp>

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
        ZoneScopedN("Actor::update");

        // suppress compiler warning
        (void)tilemap;
        (void)tileSize;

        m_lastPosition = position;
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
    }

  public: /* Fields */
    // Position in world space.
    Vector2 position;
    // Movement speed in pixels/second.
    Vector2 velocity = {0.0f, 0.0f};

  protected:
    // Position in world space on last frame.
    Vector2 m_lastPosition = position;
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
        ZoneScopedN("SpriteActor::update");

        Actor::update(deltaTime, tilemap, tileSize);
    }

    virtual void draw() const
    {
        ZoneScopedN("SpriteActor::draw");

        sprite.draw(position, scale);
    }

    virtual Rectangle get_rec(Vector2 pos) const
    {
        ZoneScopedN("SpriteActor::get_rec");

        const Texture2D &texture = sprite.get_texture();
        const Vector2 textureSize = {static_cast<float>(texture.width), static_cast<float>(texture.height)};

        return {pos.x, pos.y, textureSize.x * scale.x, textureSize.y * scale.y};
    }

    virtual Vector2 center_point(Vector2 pos)
    {
        ZoneScopedN("SpriteActor::center_point");

        const Rectangle rec = get_rec(pos);

        return {rec.x + (rec.width * 0.5f), rec.y + (rec.height * 0.5f)};
    }

    virtual Vector2 bottom_left_point(Vector2 pos) const
    {
        ZoneScopedN("SpriteActor::bottom_left_point");

        const Rectangle rec = get_rec(pos);

        return {rec.x, rec.y + rec.height};
    }

    virtual Vector2 bottom_right_point(Vector2 pos) const
    {
        ZoneScopedN("SpriteActor::bottom_right_point");

        const Rectangle rec = get_rec(pos);

        return {rec.x + rec.width, rec.y + rec.height};
    }

    virtual std::optional<GlobalTileCoord> check_tile_collision(Vector2 point, const Tilemap &tilemap,
                                                                int tileSize) const
    {
        ZoneScopedN("SpriteActor::check_tile_collision");

        const GlobalTileCoord tileCoord = point_to_tile(point, tileSize);
        const TileData *tileData = tilemap.get_tile(tileCoord);

        if (tileData and tileData->id != TileData::ID_NULL)
            return tileCoord;

        return std::nullopt;
    }

    virtual std::optional<GlobalTileCoord> touching_ground(const Tilemap &tilemap, int tileSize) const
    {
        ZoneScopedN("SpriteActor::touching_ground");

        const Vector2 leftPoint = bottom_left_point(position);
        const Vector2 rightPoint = bottom_right_point(position);
        const Vector2 leftPointLast = bottom_left_point(m_lastPosition);
        const Vector2 rightPointLast = bottom_right_point(m_lastPosition);

        std::vector<Vector2> testPoints{std::size_t{8}};

        for (float lerp = 0.0f; lerp <= 1.0f; lerp += 0.05f)
        {
            Vector2 testPointLeft = Vector2Lerp(leftPointLast, leftPoint, lerp);
            Vector2 testPointRight = Vector2Lerp(rightPointLast, rightPoint, lerp);
            testPointLeft.x += CORNER_INSET;
            testPointLeft.y += PROBE_DIST;
            testPointRight.x -= CORNER_INSET;
            testPointRight.y += PROBE_DIST;

            testPoints.emplace_back(std::move(testPointLeft));
            testPoints.emplace_back(std::move(testPointRight));
        }

        for (const Vector2 &testPoint : testPoints)
        {
            std::optional result = check_tile_collision(testPoint, tilemap, tileSize);

            if (result)
                return result;
        }

        return std::nullopt;
    }

  public:
    // Contains actor's graphical representation.
    Sprite sprite;
    // Multiplier of size relative to sprite texture.
    Vector2 scale = {1.0f, 1.0f};
};
} // namespace rotm
