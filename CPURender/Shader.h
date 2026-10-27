#pragma once

#include <vector>

#include "Vec3.h"
#include "Mesh.h"
#include "Material.h"
#include "Light.h"
#include "Color3.h"

// A fragment is the piece of surface seen through one pixel:
// everything the shader needs to know to give that pixel its colour.
// The rasterizer fills it by interpolating the attributes of the triangle's three corners.
struct Fragment
{
    math::Vector3<float> position; // world space
    math::Vector3<float> normal;   // world space, unit length
    UV uv;
};

namespace shader
{
    // The pixel (fragment) shader: final colour of one pixel, from its surface, its material and the lights.
    // The material's shading model picks the lighting equation (unlit, Lambert or Blinn-Phong).
    Color3 shade(const Fragment& fragment, const Material& material,
        const std::vector<Light>& lights, const math::Vector3<float>& cameraPosition);
}
