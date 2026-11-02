#include "Systems.h"

#include <string>

// Shows the state of the scene, on screen and in the window title.
// Everything is read from the components: a new light with a name and a key shows up on its own.

static const char* onOff(bool value)
{
    return value ? "ON" : "OFF";
}

// The P key flips every emitter at once, so they are either all on or all off.
static bool areParticlesEmitting(EntityManager& entities)
{
    const Entity entity = entities.find_first<ParticleComponent>();
    return entity != NO_ENTITY && entities.get_component<ParticleComponent>(entity).emitting;
}

static std::string describePass(EntityManager& entities)
{
    const Entity entity = entities.find_first<CameraComponent>();
    if (entity == NO_ENTITY) return "Pass: no camera";

    const CameraComponent& camera = entities.get_component<CameraComponent>(entity);
    if (!camera.transparencyEnabled) return "Pass: OPAQUE | press T to switch to transparency";

    char text[64];
    SDL_snprintf(text, sizeof(text), "Pass: TRANSPARENT | alpha = %.2f | press T to switch", camera.transparentOpacity);
    return text;
}

// "Lights: 1 sun ON | 2 bulb ON | 3 spot ON"
static std::string describeLights(EntityManager& entities)
{
    std::string text = "Lights:";
    bool first = true;

    entities.for_each<NameComponent, LightComponent, LightToggleComponent>(
        [&](Entity, const NameComponent& name, const LightComponent& light, const LightToggleComponent& toggle) {
            text += first ? " " : " | ";
            text += std::string(SDL_GetKeyName(toggle.key)) + " " + name.name + " " + onOff(light.enabled);
            first = false;
        });

    return text;
}

// "Particles: P ON | bulb 3000 | fountain 3000"
static std::string describeParticles(EntityManager& entities)
{
    std::string text = std::string("Particles: P ") + onOff(areParticlesEmitting(entities));

    entities.for_each<NameComponent, ParticleComponent>(
        [&](Entity, const NameComponent& name, const ParticleComponent& particles) {
            text += " | " + name.name + " " + std::to_string(particles.system.particles.size());
        });

    return text;
}

// Uses SDL3's built-in debug font (no additional TTF dependency required).
void systems::drawHud(EntityManager& entities, SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_RenderDebugText(renderer, 12.0f, 12.0f, "3Dzavr CPU Renderer | Shaders + Materials");
    SDL_RenderDebugText(renderer, 12.0f, 26.0f, describePass(entities).c_str());
    SDL_RenderDebugText(renderer, 12.0f, 40.0f, describeLights(entities).c_str());
    SDL_RenderDebugText(renderer, 12.0f, 54.0f, describeParticles(entities).c_str());
    SDL_RenderDebugText(renderer, 12.0f, 68.0f, "ESC: quit");
}

// "Shaders - sun ON, bulb ON, spot ON (keys 1/2/3) | opaque (key T) | particles ON (key P) | 60.0 FPS (16.67 ms)"
void systems::updateWindowTitle(EntityManager& entities, SDL_Window* window, double fps, double frameMs)
{
    std::string lights;
    std::string keys;

    entities.for_each<NameComponent, LightComponent, LightToggleComponent>(
        [&](Entity, const NameComponent& name, const LightComponent& light, const LightToggleComponent& toggle) {
            if (!lights.empty())
            {
                lights += ", ";
                keys += "/";
            }
            lights += name.name + " " + onOff(light.enabled);
            keys += SDL_GetKeyName(toggle.key);
        });

    const Entity cameraEntity = entities.find_first<CameraComponent>();
    const bool transparent = cameraEntity != NO_ENTITY && entities.get_component<CameraComponent>(cameraEntity).transparencyEnabled;

    char title[256];
    SDL_snprintf(title, sizeof(title), "Shaders - %s (keys %s) | %s (key T) | particles %s (key P) | %.1f FPS (%.2f ms)",
        lights.c_str(), keys.c_str(),
        transparent ? "transparent" : "opaque",
        onOff(areParticlesEmitting(entities)), fps, frameMs);
    SDL_SetWindowTitle(window, title);
}
