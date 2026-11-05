#include "Systems.h"

#include <format>
#include <string>
#include <vector>

// Shows the state of the scene, on screen and in the window title.
// Everything is read from the components: a new light with a name and a key shows up on its own.

static const char* on_off(bool value)
{
    return value ? "ON" : "OFF";
}

// The P key flips every emitter at once, so they are either all on or all off.
static bool are_particles_emitting(EntityManager& em, const ComponentIDs& ids)
{
    const std::vector<int> entities = em.fetch_all_entities(ids.particle);
    return !entities.empty() && static_cast<ParticleComponent*>(em.get_component(entities[0], ids.particle))->emitting;
}

static std::string describe_pass(EntityManager& em, const ComponentIDs& ids)
{
    const std::vector<int> entities = em.fetch_all_entities(ids.camera);
    if (entities.empty()) return "Pass: no camera";

    const CameraComponent& camera = *static_cast<CameraComponent*>(em.get_component(entities[0], ids.camera));
    if (!camera.transparencyEnabled) return "Pass: OPAQUE | press T to switch to transparency";

    return std::format("Pass: TRANSPARENT | alpha = {:.2f} | press T to switch", camera.transparentOpacity);
}

// "Lights: 1 sun ON | 2 bulb ON | 3 spot ON"
static std::string describe_lights(EntityManager& em, const ComponentIDs& ids)
{
    std::string text = "Lights:";
    bool first = true;

    for (int entity : em.fetch_all_entities(ids.name, ids.light, ids.lightToggle))
    {
        const NameComponent& name = *static_cast<NameComponent*>(em.get_component(entity, ids.name));
        const LightComponent& light = *static_cast<LightComponent*>(em.get_component(entity, ids.light));
        const LightToggleComponent& toggle = *static_cast<LightToggleComponent*>(em.get_component(entity, ids.lightToggle));

        text += first ? " " : " | ";
        text += std::string(SDL_GetKeyName(toggle.key)) + " " + name.name + " " + on_off(light.enabled);
        first = false;
    }

    return text;
}

// "Particles: P ON | bulb 3000 | fountain 3000"
static std::string describe_particles(EntityManager& em, const ComponentIDs& ids)
{
    std::string text = std::string("Particles: P ") + on_off(are_particles_emitting(em, ids));

    for (int entity : em.fetch_all_entities(ids.name, ids.particle))
    {
        const NameComponent& name = *static_cast<NameComponent*>(em.get_component(entity, ids.name));
        const ParticleComponent& particles = *static_cast<ParticleComponent*>(em.get_component(entity, ids.particle));
        text += " | " + name.name + " " + std::to_string(particles.system.particles.size());
    }

    return text;
}

// Uses SDL3's built-in debug font (no additional TTF dependency required).
void systems::draw_hud(EntityManager& em, const ComponentIDs& ids, SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_RenderDebugText(renderer, 12.0f, 12.0f, "3Dzavr CPU Renderer | Shaders + Materials");
    SDL_RenderDebugText(renderer, 12.0f, 26.0f, describe_pass(em, ids).c_str());
    SDL_RenderDebugText(renderer, 12.0f, 40.0f, describe_lights(em, ids).c_str());
    SDL_RenderDebugText(renderer, 12.0f, 54.0f, describe_particles(em, ids).c_str());
    SDL_RenderDebugText(renderer, 12.0f, 68.0f, "U: show / hide the UI | ESC: quit");
}

// "Shaders - sun ON, bulb ON, spot ON (keys 1/2/3) | opaque (key T) | particles ON (key P) | 60.0 FPS (16.67 ms)"
void systems::update_window_title(EntityManager& em, const ComponentIDs& ids, SDL_Window* window, double fps, double frameMs)
{
    std::string lights;
    std::string keys;

    for (int entity : em.fetch_all_entities(ids.name, ids.light, ids.lightToggle))
    {
        const NameComponent& name = *static_cast<NameComponent*>(em.get_component(entity, ids.name));
        const LightComponent& light = *static_cast<LightComponent*>(em.get_component(entity, ids.light));
        const LightToggleComponent& toggle = *static_cast<LightToggleComponent*>(em.get_component(entity, ids.lightToggle));

        if (!lights.empty())
        {
            lights += ", ";
            keys += "/";
        }
        lights += name.name + " " + on_off(light.enabled);
        keys += SDL_GetKeyName(toggle.key);
    }

    const std::vector<int> cameras = em.fetch_all_entities(ids.camera);
    const bool transparent = !cameras.empty() && static_cast<CameraComponent*>(em.get_component(cameras[0], ids.camera))->transparencyEnabled;

    const std::string title = std::format("Shaders - {} (keys {}) | {} (key T) | particles {} (key P) | {:.1f} FPS ({:.2f} ms)",
        lights, keys,
        transparent ? "transparent" : "opaque",
        on_off(are_particles_emitting(em, ids)), fps, frameMs);
    SDL_SetWindowTitle(window, title.c_str());
}
