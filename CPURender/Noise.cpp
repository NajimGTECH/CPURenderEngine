#include "Noise.h"

#include <cmath>
#include <numbers>

// Turns the position of a grid corner into a random looking number.
// Each multiplication and shift mixes the bits, so two neighbouring corners get unrelated values.
static uint32_t hash_corner(int cornerX, int cornerY, uint32_t seed)
{
    uint32_t hash = seed;
    hash ^= static_cast<uint32_t>(cornerX) * 0x27D4EB2Du;
    hash ^= static_cast<uint32_t>(cornerY) * 0x165667B1u;
    hash = (hash ^ (hash >> 15)) * 0x85EBCA6Bu;
    hash = (hash ^ (hash >> 13)) * 0xC2B2AE35u;
    return hash ^ (hash >> 16);
}

// How much a grid corner pulls the point (x, y) up or down.
// The corner has a random direction (its gradient): the dot product with the vector corner -> point
// is positive if the point lies on the side the gradient points to, negative on the other side.
static float corner_influence(int cornerX, int cornerY, float x, float y, uint32_t seed)
{
    const float angle = static_cast<float>(hash_corner(cornerX, cornerY, seed)) / 4294967296.0f * 2.0f * std::numbers::pi_v<float>;
    return std::cos(angle) * (x - static_cast<float>(cornerX)) + std::sin(angle) * (y - static_cast<float>(cornerY));
}

// 6t^5 - 15t^4 + 10t^3: goes from 0 to 1 like t, but with a flat start and a flat end.
// The slope is zero on the cell borders, so the grid doesn't show in the result.
static float fade(float t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float noise::perlin(float x, float y, uint32_t seed)
{
    // The grid cell containing the point
    const int cellX = static_cast<int>(std::floor(x));
    const int cellY = static_cast<int>(std::floor(y));

    const float blendX = fade(x - static_cast<float>(cellX));
    const float blendY = fade(y - static_cast<float>(cellY));

    // Blend the influence of the 4 corners of the cell
    const float top = std::lerp(corner_influence(cellX, cellY, x, y, seed), corner_influence(cellX + 1, cellY, x, y, seed), blendX);
    const float bottom = std::lerp(corner_influence(cellX, cellY + 1, x, y, seed), corner_influence(cellX + 1, cellY + 1, x, y, seed), blendX);

    // The blended value can't go past sqrt(2) / 2: scale it to reach [-1, 1].
    return std::lerp(top, bottom, blendY) * std::numbers::sqrt2_v<float>;
}

float noise::fractal(float x, float y, int octaves, float lacunarity, float gain, uint32_t seed)
{
    float sum = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float totalAmplitude = 0.0f;

    for (int octave = 0; octave < octaves; octave++)
    {
        // A different seed per octave: otherwise every layer repeats the same pattern at another scale
        sum += noise::perlin(x * frequency, y * frequency, seed + static_cast<uint32_t>(octave)) * amplitude;
        totalAmplitude += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }

    // Divided by the largest value the sum could reach, to stay in [-1, 1]
    return totalAmplitude > 0.0f ? sum / totalAmplitude : 0.0f;
}
