#pragma once

#include <SDL3/SDL.h>

#include "Components.h"
#include "EntityManager.h"

// The systems hold all the behaviour: each one walks the entities that have the components it cares about.
// The components are plain data, and the entities are just ids.
// None of them knows a specific entity: what an entity does only depends on the components it has.
//
// main.cpp wraps these into ECS::System lambdas and adds them to the ECS, which runs them once per frame:
//   spin -> orbit -> light sweep -> particles -> clear framebuffer -> render -> upload framebuffer -> hud -> present
// handle_key and update_window_title are not in that list: they run on an SDL event and once per second.

namespace systems
{
    // Input (Systems.cpp). Keys T and P, and the keys of the LightToggleComponents.
    void handle_key(EntityManager& em, const ComponentIDs& ids, SDL_Keycode key);

    // Simulation (Systems.cpp). seconds is the time since the start, deltaTime the time since the previous frame.
    // The order matters: the orbit moves an emitter before the particle system spawns from it.
    void spin_system(EntityManager& em, const ComponentIDs& ids, float seconds);
    void orbit_system(EntityManager& em, const ComponentIDs& ids, float seconds);
    void light_sweep_system(EntityManager& em, const ComponentIDs& ids, float seconds);
    void particle_system(EntityManager& em, const ComponentIDs& ids, float deltaTime);

    // Drawing (RenderSystem.cpp): draws every entity into the framebuffer, seen from the first camera.
    // Writes the frame and depth buffers only, nothing reaches the screen yet.
    void render(EntityManager& em, const ComponentIDs& ids);

    // On-screen text (HudSystem.cpp): the state of the lights, the particles and the camera.
    // Unlike render it draws through SDL, on top of the framebuffer once it is uploaded.
    void draw_hud(EntityManager& em, const ComponentIDs& ids, SDL_Renderer* renderer);

    // The same state in the window title, with the frame rate.
    void update_window_title(EntityManager& em, const ComponentIDs& ids, SDL_Window* window, double fps, double frameMs);
}
