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
