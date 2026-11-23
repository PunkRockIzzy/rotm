#pragma once

#include "resource_texture.hpp"

#include <memory>

#include <raylib.h>

namespace rotm
{
/**
 * @brief Holds a texture which can be easily drawn to the screen.
 *
 * @todo Add animation support via spritesheets.
 */
class Sprite
{
public:
    Sprite(std::shared_ptr<TextureResource>&& sourceTexture) : m_texture(std::move(sourceTexture))
    {
    }

    ~Sprite() = default;

    const Texture2D& get_texture() const
    {
        return m_texture->get();
    }

    void draw(Vector2 position, Vector2 scale = {1.0f, 1.0f}, float rotation = 0.0f, Color tint = WHITE) const
    {
        const Texture2D& texture = get_texture();
        const Vector2 textureSize = {static_cast<float>(texture.width), static_cast<float>(texture.height)};

        const Rectangle srcrec = {0.0f, 0.0f, (flipH ? -textureSize.x : textureSize.x),
                                  (flipV ? -textureSize.y : textureSize.y)};
        const Rectangle dstrec = {position.x, position.y, textureSize.x * scale.x, textureSize.y * scale.y};

        DrawTexturePro(texture, srcrec, dstrec, {0.0f, 0.0f}, rotation, tint);
    }

public:
    bool flipH = false;
    bool flipV = false;

private:
    std::shared_ptr<TextureResource> m_texture;
};
} // namespace rotm
