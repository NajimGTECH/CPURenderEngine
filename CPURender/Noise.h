#pragma once

#include <cstdint>

// Procedural noise: smooth random values, computed from a position instead of being stored.
// The same position and the same seed always give the same value, so a terrain can be generated again identically.

namespace noise
{
    // 2D Perlin noise, in [-1, 1]. It is 0 on every integer position, and changes smoothly in between.
    float perlin(float x, float y, uint32_t seed);

    // Fractal noise: several layers of Perlin noise (octaves) added together, in [-1, 1].
    // Each octave has its frequency multiplied by lacunarity (finer details) and its amplitude by gain (weaker details).
    float fractal(float x, float y, int octaves, float lacunarity, float gain, uint32_t seed);
}
