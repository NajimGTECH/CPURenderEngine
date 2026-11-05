#include "Chapters.h"

#include "../Framebuffer.h"
#include "../Mat4.h"
#include "../Rasterizer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>

// Chapter 11: smooth shading (Phong) + specular highlights
// The vertex normals are now computed once by the mesh loader (Mesh::vertexNormals).

constexpr float CHAPTER_11_SPEC_STRENGTH = 0.55f; // 0 = no highlight, 1 = as bright as the light

// x^32 in 5 multiplications (x^2, x^4, x^8, x^16, x^32), much cheaper than std::pow in the per-pixel loop.
// The shininess is therefore fixed at 32.
float chapter_11_pow32(float x)
{
    const float x2 = x * x;
    const float x4 = x2 * x2;
    const float x8 = x4 * x4;
    const float x16 = x8 * x8;
    return x16 * x16;
}

// Same fill as chapter 9, but the lighting is computed for every PIXEL (Phong):
// the normal is interpolated (perspective-correct, like the UVs), re-normalized,
// then we evaluate Lambert diffuse and Blinn-Phong specular.
// bilinear picks the texture sampling (chapter 12).
void chapter_11_draw_triangle_phong(ShadedVertex vA, ShadedVertex vB, ShadedVertex vC, const Texture& meshTexture,
    const math::Vector3<float>& light, const math::Vector3<float>& halfVector, bool bilinear)
{
    float area = rasterizer::edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y });
    if (area == 0.f) return;
    if (area < 0.f)
    {
        // Swap the whole vertex, so its depth, UV and normal follow its position
        std::swap(vB, vC);
        area = -area;
    }

    const Pixel pA{ vA.x, vA.y };
    const Pixel pB{ vB.x, vB.y };
    const Pixel pC{ vC.x, vC.y };

    const BoundingBox box = rasterizer::bounding_box(pA, pB, pC);

    // Like in chapter 9: every attribute interpolated under perspective is pre-divided by w,
    // normals included.
    const float uOverWA = vA.uv.u * vA.invW, vOverWA = vA.uv.v * vA.invW;
    const float uOverWB = vB.uv.u * vB.invW, vOverWB = vB.uv.v * vB.invW;
    const float uOverWC = vC.uv.u * vC.invW, vOverWC = vC.uv.v * vC.invW;
    const math::Vector3<float> nOverWA = vA.normal * vA.invW;
    const math::Vector3<float> nOverWB = vB.normal * vB.invW;
    const math::Vector3<float> nOverWC = vC.normal * vC.invW;

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

            // Depth test
            const float z = weightA * vA.z + weightB * vB.z + weightC * vC.z;
            const int index = y * WIDTH + x;
            if (z >= depthBuffer[index]) continue;

            const float invW = weightA * vA.invW + weightB * vB.invW + weightC * vC.invW;
            const float u = (weightA * uOverWA + weightB * uOverWB + weightC * uOverWC) / invW;
            const float v = (weightA * vOverWA + weightB * vOverWB + weightC * vOverWC) / invW;

            // An average of unit vectors is not unit length (it "sags" towards the centre):
            // re-normalize it, because the lighting dot products expect a unit normal.
            math::Vector3<float> normal = (nOverWA * weightA + nOverWB * weightB + nOverWC * weightC) / invW;
            normal.normalize();

            // Diffuse (chapter 10), but per pixel
            const float diffuse = std::max(0.0f, normal.dot(light));
            const float brightness = CHAPTER_10_AMBIENT + (1.0f - CHAPTER_10_AMBIENT) * diffuse;

            // Blinn-Phong specular: strongest when the normal points along the half vector
            const float specular = chapter_11_pow32(std::max(0.0f, normal.dot(halfVector))) * CHAPTER_11_SPEC_STRENGTH;

            depthBuffer[index] = z;

            // texel * diffuse, plus a white highlight on top
            Color3 texel = bilinear ? meshTexture.sample_bilinear(u, v) : meshTexture.sample(u, v);
            texel.r = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.r * brightness + 255.0f * specular), 0, 255));
            texel.g = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.g * brightness + 255.0f * specular), 0, 255));
            texel.b = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.b * brightness + 255.0f * specular), 0, 255));
            add_pixel_to_buffer(Pixel{ x, y, texel });
        }
    }
}

// Same pipeline as chapter 10, with one normal per vertex and per-pixel lighting.
void chapter_11(const Mesh& mesh, const Texture& meshTexture,
    const std::vector<math::Vector3<float>>& vertexNormals, float seconds)
{
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::perspective(fovY, aspect, 0.1f, 100.0f);

    const math::Mat4<float> view = math::Mat4<float>::look_at(
        math::Vector3<float>(0.0f, 0.0f, 3.0f),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f));

    const math::Mat4<float> model = math::Mat4<float>::rotate(
        math::Quaternion::from_axis_angle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f));
    const math::Mat4<float> modelView = model * view;

    // Same light as in chapter 10.
    const math::Vector3<float> light = math::Vector3<float>(0.4f, 0.8f, 0.45f).normalized();

    // The camera is at z = 3 looking at the origin: seen from the surface, the eye is roughly towards +Z.
    // We keep that direction constant, so the half vector H = normalize(L + V) is computed once per frame.
    const math::Vector3<float> viewDirection(0.0f, 0.0f, 1.0f);
    const math::Vector3<float> halfVector = (light + viewDirection).normalized();

    depthBuffer.fill(std::numeric_limits<float>::max());

    std::vector<ShadedVertex> projected(mesh.vertices.size());
    std::vector<bool> visible(mesh.vertices.size(), false);

    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        const math::Vector3<float> cameraPos = modelView.multiply_point_affine(mesh.vertices[i]);
        const float w = -cameraPos.z;
        if (w <= 0.0001f) continue;

        const math::Vector3<float> ndc = projection.multiply_point(cameraPos);
        projected[i].x = static_cast<int>((ndc.x + 1.0f) * 0.5f * WIDTH);
        projected[i].y = static_cast<int>((1.0f - ndc.y) * 0.5f * HEIGHT);
        projected[i].z = ndc.z;
        projected[i].invW = 1.0f / w;

        // The normal is a direction: rotate it with the model matrix, without translation.
        projected[i].normal = model.multiply_vector(vertexNormals[i]);
        visible[i] = true;
    }

    for (const Face& face : mesh.faces)
    {
        if (!visible[face.a] || !visible[face.b] || !visible[face.c]) continue;

        ShadedVertex vA = projected[face.a];
        ShadedVertex vB = projected[face.b];
        ShadedVertex vC = projected[face.c];

        // Backface culling (see chapter 9)
        if (rasterizer::edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y }) >= 0.0f) continue;

        vA.uv = (face.uvA >= 0) ? mesh.uvs[face.uvA] : UV{};
        vB.uv = (face.uvB >= 0) ? mesh.uvs[face.uvB] : UV{};
        vC.uv = (face.uvC >= 0) ? mesh.uvs[face.uvC] : UV{};

        chapter_11_draw_triangle_phong(vA, vB, vC, meshTexture, light, halfVector, false);
    }
}

