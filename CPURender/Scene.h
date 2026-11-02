#pragma once

#include <vector>

#include "EntityManager.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"

// The demo scene: what gets loaded, and which entities exist.
// This is the only place that knows about specific entities (the sun, the cows...):
// to change what is on screen, this is where to look. The systems just follow the components.

// Everything loaded from disk. Entities point to these, so they must outlive the entities.
struct Assets
{
    Mesh spot;
    Texture spotTexture; // only used by the old chapters: the materials load their own texture
    std::vector<Material> spotMaterials;
};

namespace scene
{
    // Returns false if a file can't be read.
    bool loadAssets(Assets& outAssets);

    // Registers the component types, then creates every entity of the demo.
    void create(EntityManager& entities, const Assets& assets);
}
