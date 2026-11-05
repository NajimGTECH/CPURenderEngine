#include "Editor.h"

#include "Noise.h"

#include <algorithm>
#include <array>
#include <cassert>

// Texture only: fine noise that darkens and lightens the colours, so they don't look flat.
constexpr float DETAIL_FREQUENCY = 96.0f;
constexpr float DETAIL_STRENGTH = 0.15f;

// A colour of the height ramp: at this height (in [0, 1]) the ground has exactly this colour.
struct ColorStop
{
    float height = 0.0f;
    math::Vector3<float> color;
};

// The height of the ground, as a fraction of settings.height, in [0, 1].
// gridX and gridZ are the position on the terrain, both in [0, 1].
// The mesh and the texture both read it from here: that's why the snow ends up on the peaks.
static float height_at(const TerrainSettings& settings, float gridX, float gridZ)
{
    const float value = noise::fractal(gridX * settings.frequency, gridZ * settings.frequency,
        settings.octaves, settings.lacunarity, settings.gain, settings.seed);
    return (value + 1.0f) * 0.5f;
}

// Blends between the two stops around the height.
static math::Vector3<float> height_color(float height)
{
    static const std::array<ColorStop, 4> stops = { {
        { 0.35f, math::Vector3<float>(0.22f, 0.36f, 0.14f) },   // dark grass
        { 0.50f, math::Vector3<float>(0.38f, 0.54f, 0.22f) },   // light grass
        { 0.60f, math::Vector3<float>(0.46f, 0.42f, 0.36f) },   // rock
        { 0.70f, math::Vector3<float>(0.92f, 0.92f, 0.95f) } } }; // snow

    if (height <= stops.front().height) return stops.front().color;

    for (size_t i = 0; i + 1 < stops.size(); i++)
    {
        const ColorStop& low = stops[i];
        const ColorStop& high = stops[i + 1];
        if (height < high.height)
        {
            const float t = (height - low.height) / (high.height - low.height);
            return low.color + (high.color - low.color) * t;
        }
    }

    return stops.back().color;
}

// [0, 1] colour -> [0, 255] texel colour. The detail noise can go past 1: clamp.
static Color3 to_color3(const math::Vector3<float>& color)
{
    return Color3{
        static_cast<uint8_t>(std::clamp(static_cast<int>(color.x * 255.0f), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(color.y * 255.0f), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(color.z * 255.0f), 0, 255))
    };
}

void editor::generate_terrain_mesh(const TerrainSettings& settings, Mesh& outMesh)
{
    assert(settings.resolution >= 2);
    const int side = settings.resolution;
    const float lastIndex = static_cast<float>(side - 1);

    outMesh.vertices.clear();
    outMesh.uvs.clear();
    outMesh.faces.clear();
    outMesh.vertices.reserve(static_cast<size_t>(side) * side);
    outMesh.uvs.reserve(static_cast<size_t>(side) * side);
    outMesh.faces.reserve(static_cast<size_t>(side - 1) * (side - 1) * 2);

    // The vertices, row by row. Vertex (row, column) is at index row * side + column.
    for (int row = 0; row < side; row++)
    {
        for (int column = 0; column < side; column++)
        {
            const float gridX = static_cast<float>(column) / lastIndex;
            const float gridZ = static_cast<float>(row) / lastIndex;

            outMesh.vertices.emplace_back(
                (gridX - 0.5f) * settings.size,
                height_at(settings, gridX, gridZ) * settings.height,
                (gridZ - 0.5f) * settings.size);

            // v = 0 is the BOTTOM of an image, and row 0 of the grid uses its top row
            outMesh.uvs.push_back(UV{ gridX, 1.0f - gridZ });
        }
    }

    // Each square of the grid is cut into two triangles.
    // Counter-clockwise seen from above, so their normal points up and the culling keeps them.
    for (int row = 0; row < side - 1; row++)
    {
        for (int column = 0; column < side - 1; column++)
        {
            const int topLeft = row * side + column;
            const int topRight = topLeft + 1;
            const int bottomLeft = topLeft + side;
            const int bottomRight = bottomLeft + 1;

            // A vertex and its UV have the same index
            outMesh.faces.push_back(Face{ topLeft, bottomLeft, topRight, topLeft, bottomLeft, topRight });
            outMesh.faces.push_back(Face{ topRight, bottomLeft, bottomRight, topRight, bottomLeft, bottomRight });
        }
    }

    mesh::compute_normals(outMesh);
}

void editor::generate_terrain_texture(const TerrainSettings& settings, Texture& outTexture)
{
    assert(settings.textureSize >= 1);
    const int side = settings.textureSize;

    outTexture.width = side;
    outTexture.height = side;
    outTexture.texels.resize(static_cast<size_t>(side) * side);

    for (int y = 0; y < side; y++)
    {
        for (int x = 0; x < side; x++)
        {
            // + 0.5: the centre of the texel
            const float gridX = (static_cast<float>(x) + 0.5f) / static_cast<float>(side);
            const float gridZ = (static_cast<float>(y) + 0.5f) / static_cast<float>(side);

            const float detail = noise::perlin(gridX * DETAIL_FREQUENCY, gridZ * DETAIL_FREQUENCY, settings.seed + 1000u);
            const math::Vector3<float> color = height_color(height_at(settings, gridX, gridZ)) * (1.0f + detail * DETAIL_STRENGTH);

            outTexture.texels[y * side + x] = to_color3(color);
        }
    }
}
