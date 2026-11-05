#include "Texture.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

bool Texture::load_png(const std::string& path)
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

    width = surface->w;
    height = surface->h;

    texels.resize(
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

            texels[y * surface->w + x] = Color3{
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

Color3 Texture::sample(float u, float v) const
{
    if (width <= 0 ||
        height <= 0 ||
        texels.empty())
    {
        return Color3{ 0, 0, 0, 255 };
    }

    // Wrap UV coordinates into [0, 1[.
    u -= std::floor(u);
    v -= std::floor(v);

    // OBJ coordinates have v = 0 at the bottom,
    // while image row 0 is at the top.
    const int x = std::clamp(
        static_cast<int>(u * width),
        0,
        width - 1
    );

    const int y = std::clamp(
        static_cast<int>((1.0f - v) * height),
        0,
        height - 1
    );

    return texels[y * width + x];
}

Color3 Texture::sample_bilinear(float u, float v) const
{
    if (width <= 0 ||
        height <= 0 ||
        texels.empty())
    {
        return Color3{ 0, 0, 0, 255 };
    }

    // This runs for every pixel of a textured mesh, so it avoids std::floor and std::round: they are function calls.

    // Wrap UV coordinates into [0, 1[. Most of them already are, which only costs the test.
    if (u < 0.0f || u >= 1.0f) u -= std::floor(u);
    if (v < 0.0f || v >= 1.0f) v -= std::floor(v);

    // Convert UV coordinates into texel space.
    // The half-texel offset aligns UV coordinates with texel centers.
    const float x = u * width - 0.5f;
    const float y = (1.0f - v) * height - 0.5f;

    // x and y are at least -0.5: + 1 makes them positive, where a cast to int rounds down like floor.
    const int x0 = static_cast<int>(x + 1.0f) - 1;
    const int y0 = static_cast<int>(y + 1.0f) - 1;

    const float tx = x - static_cast<float>(x0);
    const float ty = y - static_cast<float>(y0);

    // Repeat texture coordinates at the image boundaries: x0 and y0 can only go one texel too far.
    // The tests also cover any other value, which a NaN coordinate would give: the texels stay in bounds.
    const int x1 = (x0 < 0 || x0 >= width) ? width - 1 : x0;
    const int x2 = (x0 < 0 || x0 >= width - 1) ? 0 : x0 + 1;

    const int y1 = (y0 < 0 || y0 >= height) ? height - 1 : y0;
    const int y2 = (y0 < 0 || y0 >= height - 1) ? 0 : y0 + 1;

    // Four neighbouring texels.
    const Color3& c00 = texels[y1 * width + x1];
    const Color3& c10 = texels[y1 * width + x2];
    const Color3& c01 = texels[y2 * width + x1];
    const Color3& c11 = texels[y2 * width + x2];

    // How much each texel counts: the closer the point is to it, the more. They add up to 1.
    const float weight00 = (1.0f - tx) * (1.0f - ty);
    const float weight10 = tx * (1.0f - ty);
    const float weight01 = (1.0f - tx) * ty;
    const float weight11 = tx * ty;

    // Bilinear interpolation for one color channel.
    // A blend of values in [0, 255] stays in [0, 255]: + 0.5 then the cast rounds to the nearest integer.
    const auto blend = [&](uint8_t value00, uint8_t value10, uint8_t value01, uint8_t value11) -> uint8_t
        {
            return static_cast<uint8_t>(value00 * weight00 + value10 * weight10 + value01 * weight01 + value11 * weight11 + 0.5f);
        };

    return Color3{
        blend(c00.r, c10.r, c01.r, c11.r),
        blend(c00.g, c10.g, c01.g, c11.g),
        blend(c00.b, c10.b, c01.b, c11.b),
        blend(c00.a, c10.a, c01.a, c11.a)
    };
}
