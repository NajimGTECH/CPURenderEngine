#pragma once

#include <vector>

#include "EntityManager.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"

// Everything loaded from disk. Entities point to these, so they must outlive the entities.
struct Assets
{
    Mesh spot;
    Texture spotTexture; // only used by the old chapters: the materials load their own texture
    std::vector<Material> spotMaterials;
};

// The entities main.cpp needs to reach directly (key toggles, on-screen text).
struct Scene
{
    int sun = -1;
    int bulb = -1;
    int spot = -1;
    int fountain = -1;
};

namespace scene
{
    // Returns false if a file can't be read.
    bool loadAssets(Assets& outAssets);

    // Registers the component types, then creates every entity of the demo.
    Scene create(EntityManager& entities, const Assets& assets);
}
