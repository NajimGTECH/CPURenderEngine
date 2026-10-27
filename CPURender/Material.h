#pragma once

#include <string>
#include <vector>

#include "Vec3.h"
#include "Texture.h"

// How a material reacts to light: this is our "shader".
// The values follow the "illum" number of the MTL format, so a .mtl file picks it.
enum class ShadingModel
{
    Unlit = 0,     // illum 0: flat colour, lights are ignored
    Lambert = 1,   // illum 1: ambient + diffuse (chapter 10)
    BlinnPhong = 2 // illum 2: ambient + diffuse + specular highlight (chapter 11)
};

// What a surface is made of. Same data as the materials of the reference engine (3dzavr),
// read from a Wavefront .mtl file. Colours are (x, y, z) = (r, g, b) in [0, 1], as written in the file,
// so the shader can use the vector operators on them.
struct Material
{
    std::string name;                                 // newmtl

    math::Vector3<float> ambient{ 1.0f, 1.0f, 1.0f }; // Ka: how much of the ambient light is reflected
    math::Vector3<float> diffuse{ 1.0f, 1.0f, 1.0f }; // Kd: base colour of the surface
    math::Vector3<float> specular;                    // Ks: colour of the highlight, black = no highlight
    float shininess = 32.0f;                          // Ns: bigger = smaller and sharper highlight
    float opacity = 1.0f;                             // d: 1 = opaque, 0 = invisible

    ShadingModel shading = ShadingModel::Lambert;     // illum

    Texture diffuseMap;                               // map_Kd, empty if the material has no texture
};

namespace material
{
    // Minimal Wavefront MTL loader: reads newmtl, Ka, Kd, Ks, Ns, d, illum and map_Kd, ignores the rest.
    // Texture paths are relative to the .mtl file. Returns false if a file can't be read.
    bool loadMtl(const std::string& path, std::vector<Material>& outMaterials);

    bool hasTexture(const Material& material);

    // A transparent material must be drawn in the transparent pass, after the opaque ones.
    bool isTransparent(const Material& material);
}
