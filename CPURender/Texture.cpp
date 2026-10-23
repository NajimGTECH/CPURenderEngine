#include "Texture.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

bool texture::loadPng(const std::string& path, Texture& outTexture)
{
    SDL_Surface* loaded = SDL_LoadPNG(path.c_str());

    if (loaded == nullptr)
    {
        SDL_Log("Could not load %s: %s", path.c_str(), SDL_GetError());
        return false;
    }

    // Convert to ARGB8888 so every texel is one uint32_t.
    SDL_Surface* surface =
        SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_ARGB8888);

    SDL_DestroySurface(loaded);

    if (surface == nullptr)
    {
        SDL_Log("Could not convert %s: %s", path.c_str(), SDL_GetError());
        return false;
    }

    outTexture.width = surface->w;
    outTexture.height = surface->h;

    outTexture.texels.resize(
        static_cast<size_t>(surface->w) * surface->h
    );

    for (int y = 0; y < surface->h; ++y)
    {
        // Pitch is the number of bytes per row.
        const uint32_t* row = reinterpret_cast<const uint32_t*>(
            static_cast<const uint8_t*>(surface->pixels)
            + y * surface->pitch
            );

        for (int x = 0; x < surface->w; ++x)
        {
            const uint32_t argb = row[x];

            outTexture.texels[y * surface->w + x] = Color3{
                static_cast<uint8_t>((argb >> 16) & 0xFF),
                static_cast<uint8_t>((argb >> 8) & 0xFF),
                static_cast<uint8_t>(argb & 0xFF),
                static_cast<uint8_t>((argb >> 24) & 0xFF)
            };
        }
    }

    SDL_DestroySurface(surface);
    return true;
}

Color3 texture::sample(const Texture& texture, float u, float v)
{
    if (texture.width <= 0 ||
        texture.height <= 0 ||
        texture.texels.empty())
    {
        return Color3{ 0, 0, 0, 255 };
    }

    // Wrap UV coordinates into [0, 1[.
    u -= std::floor(u);
    v -= std::floor(v);

    // OBJ coordinates have v = 0 at the bottom,
    // while image row 0 is at the top.
    const int x = std::clamp(
        static_cast<int>(u * texture.width),
        0,
        texture.width - 1
    );

    const int y = std::clamp(
        static_cast<int>((1.0f - v) * texture.height),
        0,
        texture.height - 1
    );

    return texture.texels[y * texture.width + x];
}

Color3 texture::sampleBilinear(
    const Texture& texture,
    float u,
    float v)
{
    if (texture.width <= 0 ||
        texture.height <= 0 ||
        texture.texels.empty())
    {
        return Color3{ 0, 0, 0, 255 };
    }

    // Wrap UV coordinates into [0, 1[.
    u -= std::floor(u);
    v -= std::floor(v);

    // Convert UV coordinates into texel space.
    // The half-texel offset aligns UV coordinates with texel centers.
    const float x = u * texture.width - 0.5f;
    const float y = (1.0f - v) * texture.height - 0.5f;

    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));

    const float tx = x - static_cast<float>(x0);
    const float ty = y - static_cast<float>(y0);

    // Repeat texture coordinates at the image boundaries.
    const auto wrap = [](int value, int size) -> int
        {
            return ((value % size) + size) % size;
        };

    const int x1 = wrap(x0, texture.width);
    const int x2 = wrap(x0 + 1, texture.width);

    const int y1 = wrap(y0, texture.height);
    const int y2 = wrap(y0 + 1, texture.height);

    // Four neighbouring texels.
    const Color3& c00 = texture.texels[y1 * texture.width + x1];
    const Color3& c10 = texture.texels[y1 * texture.width + x2];
    const Color3& c01 = texture.texels[y2 * texture.width + x1];
    const Color3& c11 = texture.texels[y2 * texture.width + x2];

    // Linear interpolation between two values.
    const auto lerp = [](float a, float b, float t) -> float
        {
            return a + (b - a) * t;
        };

    // Bilinear interpolation for one color channel.
    const auto channel = [&](uint8_t Color3::* member) -> uint8_t
        {
            const float top = lerp(
                static_cast<float>(c00.*member),
                static_cast<float>(c10.*member),
                tx
            );

            const float bottom = lerp(
                static_cast<float>(c01.*member),
                static_cast<float>(c11.*member),
                tx
            );

            const float value = lerp(top, bottom, ty);

            return static_cast<uint8_t>(
                std::clamp(
                    static_cast<int>(std::round(value)),
                    0,
                    255
                )
                );
        };

    return Color3{
        channel(&Color3::r),
        channel(&Color3::g),
        channel(&Color3::b),
        channel(&Color3::a)
    };
}
