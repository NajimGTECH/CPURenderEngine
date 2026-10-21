#include "Texture.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>

bool texture::loadPng(const std::string& path, Texture& outTexture)
{
    SDL_Surface* loaded = SDL_LoadPNG(path.c_str());
    if (loaded == nullptr) { SDL_Log("Could not load %s: %s", path.c_str(), SDL_GetError()); return false; }

    // Convert to ARGB8888 (same layout as the framebuffer), so every texel is one uint32_t
    SDL_Surface* surface = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_ARGB8888);
    SDL_DestroySurface(loaded);
    if (surface == nullptr) { SDL_Log("Could not convert %s: %s", path.c_str(), SDL_GetError()); return false; }

    outTexture.width = surface->w;
    outTexture.height = surface->h;
    outTexture.texels.resize(static_cast<size_t>(surface->w) * surface->h);

    for (int y = 0; y < surface->h; y++)
    {
        // pitch = size of one row in bytes, it can be bigger than width * 4
        const uint32_t* row = reinterpret_cast<const uint32_t*>(
            static_cast<const uint8_t*>(surface->pixels) + y * surface->pitch);

        for (int x = 0; x < surface->w; x++)
        {
            const uint32_t argb = row[x];
            outTexture.texels[y * surface->w + x] = Color3{
                static_cast<uint8_t>(argb >> 16),
                static_cast<uint8_t>(argb >> 8),
                static_cast<uint8_t>(argb),
                static_cast<uint8_t>(argb >> 24) };
        }
    }

    SDL_DestroySurface(surface);
    return true;
}

Color3 texture::sample(const Texture& texture, float u, float v)
{
    // Wrap u and v into [0, 1[ (keep the fractional part), so we can never read outside the image
    u -= std::floor(u);
    v -= std::floor(v);

    // OBJ puts v = 0 at the BOTTOM of the image, but row 0 is the TOP: flip v
    const int x = std::clamp(static_cast<int>(u * texture.width), 0, texture.width - 1);
    const int y = std::clamp(static_cast<int>((1.0f - v) * texture.height), 0, texture.height - 1);

    return texture.texels[y * texture.width + x];
}

Color3 texture::sampleBilinear(const Texture& texture, float u, float v)
{
    // Same wrap as the nearest-neighbour sample
    u -= std::floor(u);
    v -= std::floor(v);

    // Continuous texel position (v flipped like above).
    // (size - 1) keeps u = 1 on the last texel instead of one past the edge.
    const float fx = u * (texture.width - 1);
    const float fy = (1.0f - v) * (texture.height - 1);

    const int x0 = std::clamp(static_cast<int>(std::floor(fx)), 0, texture.width - 1);
    const int y0 = std::clamp(static_cast<int>(std::floor(fy)), 0, texture.height - 1);
    const int x1 = std::min(x0 + 1, texture.width - 1);
    const int y1 = std::min(y0 + 1, texture.height - 1);

    // Fractional position of the sample inside the 4-texel cell
    const float tx = fx - std::floor(fx);
    const float ty = fy - std::floor(fy);

    const Color3& c00 = texture.texels[y0 * texture.width + x0];
    const Color3& c10 = texture.texels[y0 * texture.width + x1];
    const Color3& c01 = texture.texels[y1 * texture.width + x0];
    const Color3& c11 = texture.texels[y1 * texture.width + x1];

    // The 4 weights always sum to 1
    const float w00 = (1.0f - tx) * (1.0f - ty);
    const float w10 = tx * (1.0f - ty);
    const float w01 = (1.0f - tx) * ty;
    const float w11 = tx * ty;

    const float red = w00 * c00.r + w10 * c10.r + w01 * c01.r + w11 * c11.r;
    const float green = w00 * c00.g + w10 * c10.g + w01 * c01.g + w11 * c11.g;
    const float blue = w00 * c00.b + w10 * c10.b + w01 * c01.b + w11 * c11.b;

    return Color3{
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(red)), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(green)), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(blue)), 0, 255)) };
}
