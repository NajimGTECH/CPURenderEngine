#include "Chapters.h"

#include "../Framebuffer.h"
#include "../Mat4.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>

// Chapter 6: the 3D pipeline, on a wireframe mesh loaded from an OBJ file.

// Bresenham line: integer-only walk from one pixel to the other.
// "error" tracks how far the ideal line is from the pixel we're on,
// and tells us when to step in x, in y, or in both.
void chapter_6_draw_line(const Pixel& from, const Pixel& to)
{
    int x = from.x;
    int y = from.y;

    const int dx = std::abs(to.x - from.x);
    const int dy = -std::abs(to.y - from.y);
    const int stepX = (from.x < to.x) ? 1 : -1;
    const int stepY = (from.y < to.y) ? 1 : -1;
    int error = dx + dy;

    while (true)
    {
        add_pixel_to_buffer(Pixel{ x, y, from.color });
        if (x == to.x && y == to.y) break;

        const int doubleError = 2 * error;
        if (doubleError >= dy) { error += dy; x += stepX; }
        if (doubleError <= dx) { error += dx; y += stepY; }
    }
}

// Spinning wireframe mesh loaded from an OBJ file.
// Every vertex goes through the 3D pipeline (model -> view -> projection -> divide -> viewport),
// then each triangle is drawn as 3 lines, shaded by depth (bright = near, dim = far).
void chapter_6(const Mesh& mesh, float seconds)
{
    // Projection: 50 degrees vertical field of view
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::perspective(fovY, aspect, 0.1f, 100.0f);

    // View: camera 3 units back, looking at the origin where the mesh sits, +Y is up
    const math::Mat4<float> view = math::Mat4<float>::look_at(
        math::Vector3<float>(0.0f, 0.0f, 3.0f),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f)
    );

    // Model: spin around the Y axis
    const math::Mat4<float> model = math::Mat4<float>::rotate(
        math::Quaternion::from_axis_angle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f)
    );

    // Row vectors (p * M): model is applied first, then view.
    const math::Mat4<float> modelView = model * view;

    // Project every vertex once, faces will reuse them by index.
    struct ProjectedVertex
    {
        int x = 0, y = 0;
        float depth = 0.0f;   // distance in front of the camera (bigger = farther)
        bool visible = false; // false if behind the camera
    };

    std::vector<ProjectedVertex> projected(mesh.vertices.size());
    float minDepth = std::numeric_limits<float>::max();
    float maxDepth = std::numeric_limits<float>::lowest();

    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        // Model space -> camera space
        const math::Vector3<float> cameraPos = modelView.multiply_point_affine(mesh.vertices[i]);

        // The camera looks down -Z, so the distance in front of it is -z (this is also clip.w).
        // We shade by this depth instead of NDC z: NDC z barely changes across a small object,
        // every edge would end up the same grey.
        const float depth = -cameraPos.z;
        if (depth <= 0.0001f) continue;

        // Camera space -> NDC [-1, 1] (multiply_point does the perspective divide by w)
        const math::Vector3<float> ndc = projection.multiply_point(cameraPos);

        // NDC -> screen pixels (Y is flipped: up in NDC is down on screen)
        projected[i].x = static_cast<int>((ndc.x + 1.0f) * 0.5f * WIDTH);
        projected[i].y = static_cast<int>((1.0f - ndc.y) * 0.5f * HEIGHT);
        projected[i].depth = depth;
        projected[i].visible = true;

        minDepth = std::min(minDepth, depth);
        maxDepth = std::max(maxDepth, depth);
    }

    const float depthRange = (maxDepth > minDepth) ? (maxDepth - minDepth) : 1.0f;

    for (const Face& face : mesh.faces)
    {
        const ProjectedVertex& vA = projected[face.a];
        const ProjectedVertex& vB = projected[face.b];
        const ProjectedVertex& vC = projected[face.c];
        if (!vA.visible || !vB.visible || !vC.visible) continue;

        // Depth cue: map the mesh's own near..far span to a bright..dim grey,
        // so it stays readable whatever the distance to the camera.
        const float averageDepth = (vA.depth + vB.depth + vC.depth) / 3.0f;
        const float far01 = (averageDepth - minDepth) / depthRange; // 0 = near, 1 = far
        const uint8_t level = static_cast<uint8_t>(std::clamp(240 - static_cast<int>(far01 * 200.0f), 0, 255));
        const Color3 color{ level, level, level };

        const Pixel pA{ vA.x, vA.y, color };
        const Pixel pB{ vB.x, vB.y, color };
        const Pixel pC{ vC.x, vC.y, color };

        chapter_6_draw_line(pA, pB);
        chapter_6_draw_line(pB, pC);
        chapter_6_draw_line(pC, pA);
    }
}

