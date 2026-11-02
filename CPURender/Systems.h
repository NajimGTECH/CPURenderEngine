#pragma once

#include "EntityManager.h"

// The systems hold all the behaviour: each one walks the entities that have the components it cares about.
// The components are plain data, and the entities are just ids.

// Key T: makes every mesh see-through (alpha blending, no depth writes).
// A material can also be transparent on its own with "d" below 1 in the .mtl:
// T only caps every opacity at TRANSPARENT_OPACITY.
constexpr float TRANSPARENT_OPACITY = 0.42f;

struct RenderSettings
{
    bool transparencyEnabled = false;
};

namespace systems
{
    // Runs every simulation system, once per frame before drawing.
    // seconds is the time since the start, deltaTime the time since the previous frame.
    void update(EntityManager& entities, float seconds, float deltaTime);

    // Draws every entity into the framebuffer, seen from the first entity with a camera.
    void render(EntityManager& entities, const RenderSettings& settings);
}
