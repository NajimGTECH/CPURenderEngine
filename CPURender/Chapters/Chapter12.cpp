#include "Chapters.h"

#include "../Framebuffer.h"
#include "../Mat4.h"
#include "../Rasterizer.h"

#include <algorithm>
#include <limits>
#include <numbers>
#include <vector>

// Chapter 12: clipping against the near plane + bilinear filtering
// The clipping itself now lives in the rasterizer: the live renderer still uses it.

// Set to false to go back to the blocky "nearest neighbour" sampling of chapter 9.
constexpr bool CHAPTER_12_BILINEAR = true;

// Distance between the camera and Spot. Set it to 0.4f to go inside the cow:
// the surface stays solid right up to the cut, instead of losing whole triangles.
constexpr float CHAPTER_12_CAMERA_DISTANCE = 3.0f;

// An already clipped triangle (entirely in front of the near plane, so w > 0):
// divide by w, viewport, backface culling, then the Phong rasterizer of chapter 11.
void chapter_12_project_and_draw(const ClipVertex triangle[3], const Texture& meshTexture,
    const math::Vector3<float>& light, const math::Vector3<float>& halfVector)
{
    ShadedVertex projected[3];
    for (int k = 0; k < 3; k++)
    {
        const float invW = 1.0f / triangle[k].w;
        projected[k].x = static_cast<int>((triangle[k].x * invW + 1.0f) * 0.5f * WIDTH);
        projected[k].y = static_cast<int>((1.0f - triangle[k].y * invW) * 0.5f * HEIGHT);
        projected[k].z = triangle[k].z * invW;
        projected[k].invW = invW;
        projected[k].uv = triangle[k].uv;
        projected[k].normal = triangle[k].normal;
    }

    // Backface culling (see chapter 9)
    if (rasterizer::edge(Pixel{ projected[0].x, projected[0].y }, Pixel{ projected[1].x, projected[1].y },
        Pixel{ projected[2].x, projected[2].y }) >= 0.0f) return;

    chapter_11_draw_triangle_phong(projected[0], projected[1], projected[2], meshTexture, light, halfVector, CHAPTER_12_BILINEAR);
}

// Same render as chapter 11, but the pipeline is reorganized around clipping:
//  1) per mesh vertex: only the move to clip space and the world normal (no divide);
//  2) per triangle: near clipping -> 0, 1 or 2 triangles, then divide + viewport + culling + raster.
// Clipping can create NEW vertices (the crossing points), which is why
// the divide by w can no longer be done once per mesh vertex.
void chapter_12(const Mesh& mesh, const Texture& meshTexture,
    const std::vector<math::Vector3<float>>& vertexNormals, float seconds)
{
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::perspective(fovY, aspect, 0.1f, 100.0f);

    const math::Mat4<float> view = math::Mat4<float>::look_at(
        math::Vector3<float>(0.0f, 0.0f, CHAPTER_12_CAMERA_DISTANCE),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f));

    const math::Mat4<float> model = math::Mat4<float>::rotate(
        math::Quaternion::from_axis_angle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f));

    // Row vectors (p * M): model, then view, then projection.
    const math::Mat4<float> modelViewProjection = model * view * projection;

    const math::Vector3<float> light = math::Vector3<float>(0.4f, 0.8f, 0.45f).normalized();
    const math::Vector3<float> viewDirection(0.0f, 0.0f, 1.0f);
    const math::Vector3<float> halfVector = (light + viewDirection).normalized();

    std::fill_n(depthBuffer, WIDTH * HEIGHT, std::numeric_limits<float>::max());

    // 1) Per vertex: clip space + world normal. No more need for the "visible" array:
    // clipping properly handles whatever is behind the camera.
    std::vector<ClipVertex> clipVertices(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        clipVertices[i] = rasterizer::to_clip_space(modelViewProjection, mesh.vertices[i]);
        clipVertices[i].normal = model.multiply_vector(vertexNormals[i]);
    }

    // 2) Per triangle
    for (const Face& face : mesh.faces)
    {
        ClipVertex vA = clipVertices[face.a];
        ClipVertex vB = clipVertices[face.b];
        ClipVertex vC = clipVertices[face.c];

        vA.uv = (face.uvA >= 0) ? mesh.uvs[face.uvA] : UV{};
        vB.uv = (face.uvB >= 0) ? mesh.uvs[face.uvB] : UV{};
        vC.uv = (face.uvC >= 0) ? mesh.uvs[face.uvC] : UV{};

        ClipVertex clipped[6];
        const int triangleCount = rasterizer::clip_triangle_near(vA, vB, vC, clipped);
        for (int t = 0; t < triangleCount; t++)
        {
            chapter_12_project_and_draw(&clipped[t * 3], meshTexture, light, halfVector);
        }
    }
}

