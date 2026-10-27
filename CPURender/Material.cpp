#include "Material.h"

#include <SDL3/SDL_log.h>

#include <fstream>
#include <sstream>

// "Ka r g b", "Kd r g b", "Ks r g b": three numbers in [0, 1]
static void readColor(std::istringstream& stream, math::Vector3<float>& color)
{
    math::Vector3<float> value;
    if (stream >> value.x >> value.y >> value.z)
    {
        color = value;
    }
}

// The MTL format defines illum 0 to 10. We support the first three,
// the others (reflections, refraction, ...) fall back to the most complete one we have.
static ShadingModel shadingFromIllum(int illum)
{
    if (illum <= 0) return ShadingModel::Unlit;
    if (illum == 1) return ShadingModel::Lambert;
    return ShadingModel::BlinnPhong;
}

bool material::loadMtl(const std::string& path, std::vector<Material>& outMaterials)
{
    std::ifstream file(path);
    if (!file.is_open()) { SDL_Log("Could not open %s", path.c_str()); return false; }

    outMaterials.clear();

    // Texture file names are relative to the .mtl file, so keep its folder
    const size_t lastSlash = path.find_last_of("/\\");
    const std::string folder = (lastSlash == std::string::npos) ? "" : path.substr(0, lastSlash + 1);

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream stream(line);
        std::string type;
        stream >> type;

        // "newmtl name" starts a new material: every line until the next "newmtl" describes it
        if (type == "newmtl")
        {
            outMaterials.emplace_back();
            stream >> outMaterials.back().name;
            continue;
        }

        // Lines before the first "newmtl" don't belong to any material
        if (outMaterials.empty()) continue;
        Material& material = outMaterials.back();

        if (type == "Ka") readColor(stream, material.ambient);
        else if (type == "Kd") readColor(stream, material.diffuse);
        else if (type == "Ks") readColor(stream, material.specular);
        else if (type == "Ns") stream >> material.shininess;
        else if (type == "d") stream >> material.opacity;
        else if (type == "illum")
        {
            int illum = 1;
            if (stream >> illum) material.shading = shadingFromIllum(illum);
        }
        else if (type == "map_Kd")
        {
            std::string fileName;
            if (stream >> fileName && !texture::loadPng(folder + fileName, material.diffuseMap)) return false;
        }
        // Any other line type (#comments, Ke, Ni, map_Bump, ...) is ignored
    }

    return true;
}

bool material::hasTexture(const Material& material)
{
    return !material.diffuseMap.texels.empty();
}

bool material::isTransparent(const Material& material)
{
    return material.opacity < 1.0f;
}
