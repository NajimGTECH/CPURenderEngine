#pragma once

#include <cstdint>

#include "Mesh.h"
#include "Texture.h"

// The editor module: tools that create content instead of loading it from a file.
// It only fills a Mesh and a Texture, so a generated terrain is drawn like any other mesh:
// give them to an entity, and the render system doesn't see the difference.

// What a terrain looks like. The same settings always give the same terrain: change the seed to get another one.
struct TerrainSettings
{
    int resolution = 96;    // vertices per side of the grid, at least 2
    float size = 20.0f;     // length of a side on X and Z, in world units
    float height = 6.0f;    // distance between the lowest and the highest possible point, in world units

    float frequency = 4.0f; // noise cells across the terrain: bigger = more hills, closer together
    int octaves = 5;        // layers of noise: more = more small details
    float lacunarity = 2.0f;
    float gain = 0.5f;
    uint32_t seed = 1234u;

    int textureSize = 512;  // texels per side of the texture
};

namespace editor
{
    // A grid of vertices centered on X and Z, each one raised by the noise. y goes from 0 to settings.height.
    // Replaces the content of outMesh.
    void generate_terrain_mesh(const TerrainSettings& settings, Mesh& outMesh);

    // One image stretched over the whole terrain, each texel coloured by the height of the ground under it:
    // grass in the valleys, then rock, then snow on the peaks. Replaces the content of outTexture.
    void generate_terrain_texture(const TerrainSettings& settings, Texture& outTexture);
}
