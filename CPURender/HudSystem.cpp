#include "Systems.h"

#include <string>
#include <vector>

// Shows the state of the scene, on screen and in the window title.
// Everything is read from the components: a new light with a name and a key shows up on its own.

static const char* onOff(bool value)
{
    return value ? "ON" : "OFF";
}

// The P key flips every emitter at once, so they are either all on or all off.
static bool areParticlesEmitting(EntityManager& em, const ComponentIDs& ids)
{
    const std::vector<int> entities = em.FetchAllEntities(ids.particle);
    return !entities.empty() && static_cast<ParticleComponent*>(em.GetComponent(entities[0], ids.particle))->emitting;
}

static std::string describePass(EntityManager& em, const ComponentIDs& ids)
{
    const std::vector<int> entities = em.FetchAllEntities(ids.camera);
    if (entities.empty()) return "Pass: no camera";

    const CameraComponent& camera = *static_cast<CameraComponent*>(em.GetComponent(entities[0], ids.camera));
    if (!camera.transparencyEnabled) return "Pass: OPAQUE | press T to switch to transparency";

    char text[64];
    SDL_snprintf(text, sizeof(text), "Pass: TRANSPARENT | alpha = %.2f | press T to switch", camera.transparentOpacity);
    return text;
}

// "Lights: 1 sun ON | 2 bulb ON | 3 spot ON"
static std::string describeLights(EntityManager& em, const ComponentIDs& ids)
{
    std::string text = "Lights:";
    bool first = true;

    for (int entity : em.FetchAllEntities(ids.name, ids.light, ids.lightToggle))
    {
        const NameComponent& name = *static_cast<NameComponent*>(em.GetComponent(entity, ids.name));
        const LightComponent& light = *static_cast<LightComponent*>(em.GetComponent(entity, ids.light));
        const LightToggleComponent& toggle = *static_cast<LightToggleComponent*>(em.GetComponent(entity, ids.lightToggle));

        text += first ? " " : " | ";
        text += std::string(SDL_GetKeyName(toggle.key)) + " " + name.name + " " + onOff(light.enabled);
        first = false;
    }

    return text;
}

// "Particles: P ON | bulb 3000 | fountain 3000"
static std::string describeParticles(EntityManager& em, const ComponentIDs& ids)
{
    std::string text = std::string("Particles: P ") + onOff(areParticlesEmitting(em, ids));

    for (int entity : em.FetchAllEntities(ids.name, ids.particle))
    {
        const NameComponent& name = *static_cast<NameComponent*>(em.GetComponent(entity, ids.name));
        const ParticleComponent& particles = *static_cast<ParticleComponent*>(em.GetComponent(entity, ids.particle));
        text += " | " + name.name + " " + std::to_string(particles.system.particles.size());
    }

    return text;
}

// Uses SDL3's built-in debug font (no additional TTF dependency required).
void systems::drawHud(EntityManager& em, const ComponentIDs& ids, SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_RenderDebugText(renderer, 12.0f, 12.0f, "3Dzavr CPU Renderer | Shaders + Materials");
    SDL_RenderDebugText(renderer, 12.0f, 26.0f, describePass(em, ids).c_str());
    SDL_RenderDebugText(renderer, 12.0f, 40.0f, describeLights(em, ids).c_str());
    SDL_RenderDebugText(renderer, 12.0f, 54.0f, describeParticles(em, ids).c_str());
    SDL_RenderDebugText(renderer, 12.0f, 68.0f, "ESC: quit");
}

// "Shaders - sun ON, bulb ON, spot ON (keys 1/2/3) | opaque (key T) | particles ON (key P) | 60.0 FPS (16.67 ms)"
void systems::updateWindowTitle(EntityManager& em, const ComponentIDs& ids, SDL_Window* window, double fps, double frameMs)
{
    std::string lights;
    std::string keys;

    for (int entity : em.FetchAllEntities(ids.name, ids.light, ids.lightToggle))
    {
        const NameComponent& name = *static_cast<NameComponent*>(em.GetComponent(entity, ids.name));
        const LightComponent& light = *static_cast<LightComponent*>(em.GetComponent(entity, ids.light));
        const LightToggleComponent& toggle = *static_cast<LightToggleComponent*>(em.GetComponent(entity, ids.lightToggle));

        if (!lights.empty())
        {
            lights += ", ";
            keys += "/";
        }
        lights += name.name + " " + onOff(light.enabled);
        keys += SDL_GetKeyName(toggle.key);
    }

    const std::vector<int> cameras = em.FetchAllEntities(ids.camera);
    const bool transparent = !cameras.empty() && static_cast<CameraComponent*>(em.GetComponent(cameras[0], ids.camera))->transparencyEnabled;

    char title[256];
    SDL_snprintf(title, sizeof(title), "Shaders - %s (keys %s) | %s (key T) | particles %s (key P) | %.1f FPS (%.2f ms)",
        lights.c_str(), keys.c_str(),
        transparent ? "transparent" : "opaque",
        onOff(areParticlesEmitting(em, ids)), fps, frameMs);
    SDL_SetWindowTitle(window, title);
}
