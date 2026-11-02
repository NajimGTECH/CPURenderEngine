#pragma once

#include <vector>

#include "Light.h"
#include "Mat4.h"
#include "Material.h"
#include "Mesh.h"
#include "Particle.h"
#include "Pixel.h"
#include "Vec3.h"

// A vertex in clip space (after the projection, BEFORE the divide by w),
// with every attribute the clipping must interpolate and the rasterizer needs afterwards.
struct ClipVertex
{
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f; // clip space position
    UV uv;                                        // texture coordinate
    math::Vector3<float> normal;                  // world space normal
    math::Vector3<float> worldPosition;           // world space position (for the shaders)
};

namespace rasterizer
{
    // The edge function: which side of edge A->B the point P is on (chapter 3).
    float edge(const Pixel& a, const Pixel& b, const Pixel& p);

    // p * M, keeping w: the divide must wait until the triangle is clipped (chapter 12).
    ClipVertex toClipSpace(const math::Mat4<float>& matrix, const math::Vector3<float>& p);

    // Clips a triangle against the near plane. Writes 0, 3 or 6 vertices to out
    // and returns the number of triangles (0, 1 or 2) (chapter 12).
    int clipTriangleNear(const ClipVertex& vA, const ClipVertex& vB, const ClipVertex& vC, ClipVertex out[6]);

    // Draws a mesh placed in the world by its model matrix, with a material.
    // opacity < 1 draws the mesh in the transparent pass: call it AFTER every opaque mesh.
    void drawMesh(const Mesh& mesh,
        const math::Mat4<float>& model, const math::Mat4<float>& viewProjection, const Material& material,
        const std::vector<Light>& lights, const math::Vector3<float>& cameraPosition, float opacity);

    // Draws every particle of a system as one pixel.
    void drawParticles(const ParticleSystem& system, const math::Mat4<float>& viewProjection);
}
