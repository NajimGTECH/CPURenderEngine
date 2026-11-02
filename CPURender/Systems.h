#pragma once

#include <SDL3/SDL.h>

#include "EntityManager.h"

// The systems hold all the behaviour: each one walks the entities that have the components it cares about.
// The components are plain data, and the entities are just ids.
// None of them knows a specific entity: what an entity does only depends on the components it has.
//
// main.cpp calls these once per frame, in this order:
//   handleKey -> update -> render -> drawHud

namespace systems
{
    // Input (Systems.cpp). Keys T and P, and the keys of the LightToggleComponents.
    void handleKey(EntityManager& entities, SDL_Keycode key);

    // Simulation (Systems.cpp): runs spin, orbit, light sweep and particles.
    // seconds is the time since the start, deltaTime the time since the previous frame.
    void update(EntityManager& entities, float seconds, float deltaTime);

    // Drawing (RenderSystem.cpp): draws every entity into the framebuffer, seen from the first camera.
    // Writes the frame and depth buffers only, nothing reaches the screen yet.
    void render(EntityManager& entities);

    // On-screen text (HudSystem.cpp): the state of the lights, the particles and the camera.
    // Unlike render it draws through SDL, on top of the framebuffer once it is uploaded.
    void drawHud(EntityManager& entities, SDL_Renderer* renderer);

    // The same state in the window title, with the frame rate.
    void updateWindowTitle(EntityManager& entities, SDL_Window* window, double fps, double frameMs);
}
