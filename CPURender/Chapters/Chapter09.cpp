#include "Chapters.h"

#include "../Framebuffer.h"
#include "../Mat4.h"
#include "../Rasterizer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>

// Chapter 9: textures, with perspective-correct UVs.

// Same barycentric fill as chapter 3, but instead of blending 3 colors we blend 3 texture
// coordinates, then read the color of the texture at that spot.
void chapter_9_draw_triangle_textured(TexturedVertex vA, TexturedVertex vB, TexturedVertex vC, const Texture& meshTexture)
{
    float area = rasterizer::edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y });
    if (area == 0.f) return;
    if (area < 0.f)
    {
        // Swap the whole vertex, so its depth and UV follow its position
        std::swap(vB, vC);
        area = -area;
    }

    const Pixel pA{ vA.x, vA.y };
    const Pixel pB{ vB.x, vB.y };
    const Pixel pC{ vC.x, vC.y };

    const BoundingBox box = rasterizer::bounding_box(pA, pB, pC);

    // u and v do NOT vary linearly across the screen under perspective, but u/w, v/w and 1/w do.
    // So we pre-divide the UVs by w here, and divide back at each pixel.
    const float uOverWA = vA.uv.u * vA.invW, vOverWA = vA.uv.v * vA.invW;
    const float uOverWB = vB.uv.u * vB.invW, vOverWB = vB.uv.v * vB.invW;
    const float uOverWC = vC.uv.u * vC.invW, vOverWC = vC.uv.v * vC.invW;

    for (int y = box.minY; y <= box.maxY; y++)
    {
        for (int x = box.minX; x <= box.maxX; x++)
        {
            const Pixel point{ x, y };
            const float edgeA = rasterizer::edge(pB, pC, point);
            const float edgeB = rasterizer::edge(pC, pA, point);
            const float edgeC = rasterizer::edge(pA, pB, point);

            // Outside the triangle
            if (edgeA < 0 || edgeB < 0 || edgeC < 0) continue;

            const float weightA = edgeA / area;
            const float weightB = edgeB / area;
            const float weightC = edgeC / area;

            // Depth test: only draw if this pixel is closer than what's already there.
            // NDC z already varies linearly across the screen, so a plain blend is correct.
            const float z = weightA * vA.z + weightB * vB.z + weightC * vC.z;
            const int index = y * WIDTH + x;
            if (z >= depthBuffer[index]) continue;

            float u, v;
            if (CHAPTER_9_PERSPECTIVE_CORRECT)
            {
                // Blend u/w, v/w and 1/w, then divide back to get the true u, v
                const float invW = weightA * vA.invW + weightB * vB.invW + weightC * vC.invW;
                u = (weightA * uOverWA + weightB * uOverWB + weightC * uOverWC) / invW;
                v = (weightA * vOverWA + weightB * vOverWB + weightC * vOverWC) / invW;
            }
            else
            {
                // Naive blend, wrong under perspective
                u = weightA * vA.uv.u + weightB * vB.uv.u + weightC * vC.uv.u;
                v = weightA * vA.uv.v + weightB * vB.uv.v + weightC * vC.uv.v;
            }

            depthBuffer[index] = z;
            add_pixel_to_buffer(Pixel{ x, y, texture::sample(meshTexture, u, v) });
        }
    }
}

// Spinning textured mesh: filled triangles, a depth buffer so near triangles hide far ones,
// backface culling, and a texture wrapped on it with perspective-correct UVs.
void chapter_9(const Mesh& mesh, const Texture& meshTexture, float seconds)
{
    // Same camera and spin as chapter 6
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::perspective(fovY, aspect, 0.1f, 100.0f);

    const math::Mat4<float> view = math::Mat4<float>::look_at(
        math::Vector3<float>(0.0f, 0.0f, 3.0f),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f)
    );

    const math::Mat4<float> model = math::Mat4<float>::rotate(
        math::Quaternion::from_axis_angle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f)
    );

    const math::Mat4<float> modelView = model * view;

    // Reset the depth buffer: nothing drawn yet = infinitely far
    depthBuffer.fill(std::numeric_limits<float>::max());

    // Project every vertex once, faces will reuse them by index.
    std::vector<TexturedVertex> projected(mesh.vertices.size());
    std::vector<bool> visible(mesh.vertices.size(), false);

    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        // Model space -> camera space
        const math::Vector3<float> cameraPos = modelView.multiply_point_affine(mesh.vertices[i]);

        // The camera looks down -Z, so w (the distance in front of it) is -z
        const float w = -cameraPos.z;
        if (w <= 0.0001f) continue;

        // Camera space -> NDC [-1, 1] (multiply_point does the perspective divide by w)
        const math::Vector3<float> ndc = projection.multiply_point(cameraPos);

        // NDC -> screen pixels (Y is flipped: up in NDC is down on screen)
        projected[i].x = static_cast<int>((ndc.x + 1.0f) * 0.5f * WIDTH);
        projected[i].y = static_cast<int>((1.0f - ndc.y) * 0.5f * HEIGHT);
        projected[i].z = ndc.z;
        projected[i].invW = 1.0f / w;
        visible[i] = true;
    }

    for (const Face& face : mesh.faces)
    {
        if (!visible[face.a] || !visible[face.b] || !visible[face.c]) continue;

        TexturedVertex vA = projected[face.a];
        TexturedVertex vB = projected[face.b];
        TexturedVertex vC = projected[face.c];

        // Backface culling: OBJ front faces are counter-clockwise, and the Y flip of the viewport
        // makes them clockwise on screen, so their area is negative.
        // Positive area = we're looking at the back of the triangle, it's hidden anyway: skip it.
        if (rasterizer::edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y }) >= 0.f) continue;

        // The UV belongs to the face corner, not the vertex: the same vertex can have
        // different UVs on two faces (on a seam of the texture)
        vA.uv = (face.uvA >= 0) ? mesh.uvs[face.uvA] : UV{};
        vB.uv = (face.uvB >= 0) ? mesh.uvs[face.uvB] : UV{};
        vC.uv = (face.uvC >= 0) ? mesh.uvs[face.uvC] : UV{};

        chapter_9_draw_triangle_textured(vA, vB, vC, meshTexture);
    }
}

