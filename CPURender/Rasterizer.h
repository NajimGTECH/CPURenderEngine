#pragma once

#include <array>
#include <vector>

#include "Light.h"
#include "Mat4.h"
#include "Material.h"
#include "Mesh.h"
#include "Particle.h"
#include "Pixel.h"
#include "Vec3.h"

// The live drawing pipeline: everything the chapters taught that the engine still runs on.
// It knows nothing about entities: the render system (RenderSystem.cpp) feeds it one mesh at a time.
//
// What draw_mesh does to a mesh:
//   vertices -> clip space -> near plane clipping -> divide by w + viewport -> backface culling
//            -> for each pixel of the triangle: depth test -> shader::shade -> framebuffer

// A vertex in clip space (after the projection, BEFORE the divide by w),
// with every attribute the clipping must interpolate and the rasterizer needs afterwards.
struct ClipVertex
{
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f; // clip space position
    UV uv;                                        // texture coordinate
    math::Vector3<float> normal;                  // world space normal
    math::Vector3<float> worldPosition;           // world space position (for the shaders)
};

// A rectangle of pixels. Both corners are included, and min > max means it is empty.
// Used for the pixels a triangle or a mesh can cover, and for the tiles of the screen (Tiles.h).
struct BoundingBox
{
    int minX = 0, minY = 0;
    int maxX = 0, maxY = 0;
};

namespace rasterizer
{
    // The edge function: which side of edge A->B the point P is on (chapter 3).
    float edge(const Pixel& a, const Pixel& b, const Pixel& p);

    // The smallest rectangle around a triangle, clamped to the screen (chapter 3).
    BoundingBox bounding_box(const Pixel& a, const Pixel& b, const Pixel& c);

    // The pixels that are in both rectangles. Empty if they don't overlap.
    BoundingBox intersect(const BoundingBox& a, const BoundingBox& b);

    // True if the rectangle has no pixel at all.
    bool is_empty(const BoundingBox& box);

    // The rectangle of the screen a mesh can cover once placed by its model matrix. Empty if it is off screen.
    BoundingBox screen_box(const Mesh& mesh, const math::Mat4<float>& model, const math::Mat4<float>& viewProjection);

    // p * M, keeping w: the divide must wait until the triangle is clipped (chapter 12).
    ClipVertex to_clip_space(const math::Mat4<float>& matrix, const math::Vector3<float>& p);

    // Clips a triangle against the near plane. Writes 0, 3 or 6 vertices to out
    // and returns the number of triangles (0, 1 or 2) (chapter 12).
    int clip_triangle_near(const ClipVertex& vA, const ClipVertex& vB, const ClipVertex& vC, std::array<ClipVertex, 6>& out);

    // The draw functions below only write the pixels inside region, and leave the rest of the screen untouched.
    // Pass the whole screen to draw everything, or one tile of it: two calls with regions that don't overlap
    // never write the same pixel or depth value, so they can run at the same time on two threads.

    // Draws a mesh placed in the world by its model matrix, with a material.
    // opacity < 1 draws the mesh in the transparent pass: call it AFTER every opaque mesh.
    void draw_mesh(const Mesh& mesh,
        const math::Mat4<float>& model, const math::Mat4<float>& viewProjection, const Material& material,
        const std::vector<Light>& lights, const math::Vector3<float>& cameraPosition, float opacity,
        const BoundingBox& region);

    // Draws every particle of a system as one pixel.
    void draw_particles(const ParticleSystem& system, const math::Mat4<float>& viewProjection, const BoundingBox& region);
}
