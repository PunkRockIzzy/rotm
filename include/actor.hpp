#pragma once

#include "resource_texture.hpp"
#include "sprite.hpp"
#include <memory>
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

    ~Actor() = default;

    Actor() = delete;

    Actor(const Actor &) = delete;
    Actor &operator=(const Actor &) = delete;

    Actor(Actor &&) noexcept = default;
    Actor &operator=(Actor &&) noexcept = default;

    virtual void update(double deltaTime)
    {
        (void)deltaTime; // suppress compiler warning
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
  public: /* Methods */
    /**
     * @brief Construct a new Sprite Actor object at startPos, with sprite using spriteTexture.
     *
     * @param startPos Initial position in world space.
     * @param spriteTexture Texture for initializing sprite.
     */
    SpriteActor(Vector2 startPos, std::shared_ptr<TextureResource> &&spriteTexture)
        : Actor(startPos), m_sprite(std::move(spriteTexture))
    {
    }

    /**
     * @brief Trivial accessor for m_sprite.
     *
     * @return const Sprite& Reference to m_sprite.
     */
    virtual const Sprite &get_sprite() const
    {
        return m_sprite;
    }

    virtual void draw() const
    {
        m_sprite.draw(position, scale);
    }

    virtual Rectangle get_rec() const
    {
        const Texture2D &texture = m_sprite.get_texture();
        const Vector2 textureSize = {static_cast<float>(texture.width), static_cast<float>(texture.height)};

        return {position.x, position.y, textureSize.x * scale.x, textureSize.y * scale.y};
    }

  public:
    // Multiplier of size relative to sprite texture.
    Vector2 scale = {1.0f, 1.0f};

  private: /* Fields */
    Sprite m_sprite;
};
} // namespace rotm
