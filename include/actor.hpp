#pragma once

#include "resource_texture.hpp"
#include "sprite.hpp"
#include "tile_math.hpp"
#include "tilemap.hpp"

// #include <cmath>
#include <cmath>
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

    virtual Vector2 top_left_point(Vector2 pos) const
    {
        ZoneScopedN("SpriteActor::top_left_point");

        const Rectangle rec = get_rec(pos);

        return {rec.x, rec.y};
    }

    virtual Vector2 top_right_point(Vector2 pos) const
    {
        ZoneScopedN("SpriteActor::top_right_point");

        const Rectangle rec = get_rec(pos);

        return {rec.x + rec.width, rec.y};
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

    virtual std::optional<GlobalTileCoord> check_surface_collision(Vector2 pos, Vector2 posLast, const Tilemap &tilemap,
                                                                   int tileSize) const
    {
        static constexpr std::size_t SUBDIVISIONS = 20;
        static constexpr double LERP_INCREMENT = 1.0 / static_cast<double>(SUBDIVISIONS);

        ZoneScopedN("SpriteActor::check_surface_collision");

        std::vector<Vector2> testPoints;
        testPoints.reserve(SUBDIVISIONS);

        for (float lerp = 0.0f; lerp <= 1.0f; lerp += LERP_INCREMENT)
        {
            const float x = std::lerp(posLast.x, pos.x, lerp);
            const float y = std::lerp(posLast.y, pos.y, lerp);

            testPoints.emplace_back(x, y);
        }

        for (const Vector2 &testPoint : testPoints)
        {
            std::optional<GlobalTileCoord> result = check_tile_collision(testPoint, tilemap, tileSize);

            if (result)
                return result;
        }

        return std::nullopt;
    }

    virtual std::optional<GlobalTileCoord> touching_ground(const Tilemap &tilemap, int tileSize) const
    {
        ZoneScopedN("SpriteActor::touching_ground");

        Vector2 leftPoint = bottom_left_point(position);
        Vector2 leftPointLast = bottom_left_point(m_lastPosition);
        leftPoint.x += CORNER_INSET;
        leftPointLast.x += CORNER_INSET;
        leftPoint.y += PROBE_DIST;
        leftPointLast.y += PROBE_DIST;

        if (auto leftResult = check_surface_collision(leftPoint, leftPointLast, tilemap, tileSize))
            return leftResult;

        Vector2 rightPoint = bottom_right_point(position);
        Vector2 rightPointLast = bottom_right_point(m_lastPosition);
        rightPoint.x -= CORNER_INSET;
        rightPointLast.x -= CORNER_INSET;
        rightPoint.y += PROBE_DIST;
        rightPointLast.y += PROBE_DIST;

        if (auto rightResult = check_surface_collision(rightPoint, rightPointLast, tilemap, tileSize))
            return rightResult;

        return std::nullopt;
    }

    virtual std::optional<GlobalTileCoord> touching_ceiling(const Tilemap &tilemap, int tileSize)
    {
        ZoneScopedN("SpriteActor::touching_ceiling");

        Vector2 leftPoint = top_left_point(position);
        Vector2 leftPointLast = top_left_point(m_lastPosition);
        leftPoint.x += CORNER_INSET;
        leftPointLast.x += CORNER_INSET;
        leftPoint.y -= PROBE_DIST;
        leftPointLast.y -= PROBE_DIST;

        if (auto leftResult = check_surface_collision(leftPoint, leftPointLast, tilemap, tileSize))
            return leftResult;

        Vector2 rightPoint = top_right_point(position);
        Vector2 rightPointLast = top_right_point(m_lastPosition);
        rightPoint.x -= CORNER_INSET;
        rightPointLast.x -= CORNER_INSET;
        rightPoint.y -= PROBE_DIST;
        rightPointLast.y -= PROBE_DIST;

        if (auto rightResult = check_surface_collision(rightPoint, rightPointLast, tilemap, tileSize))
            return rightResult;

        return std::nullopt;
    }

  public:
    // Contains actor's graphical representation.
    Sprite sprite;
    // Multiplier of size relative to sprite texture.
    Vector2 scale = {1.0f, 1.0f};
};
} // namespace rotm
