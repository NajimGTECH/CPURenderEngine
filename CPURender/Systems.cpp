#include "Systems.h"

#include <cmath>

// The input and simulation systems. Drawing is in RenderSystem.cpp, the on-screen text in HudSystem.cpp.

// ---- Input ----

void systems::handleKey(EntityManager& entities, SDL_Keycode key)
{
    // T: switches the camera between opaque and see-through meshes.
    if (key == SDLK_T)
    {
        entities.for_each<CameraComponent>([](Entity, CameraComponent& camera) {
            camera.transparencyEnabled = !camera.transparencyEnabled;
            });
    }

    // P: stops / restarts the emitters. The particles already alive finish their life.
    if (key == SDLK_P)
    {
        entities.for_each<ParticleComponent>([](Entity, ParticleComponent& particles) {
            particles.emitting = !particles.emitting;
            });
    }

    // Each light can have its own key, to see what it brings.
    entities.for_each<LightComponent, LightToggleComponent>(
        [&](Entity, LightComponent& light, const LightToggleComponent& toggle) {
            if (toggle.key == key) light.enabled = !light.enabled;
        });
}


// ---- Simulation ----

// Spin: the rotation only depends on the time since the start.
static void spinSystem(EntityManager& entities, float seconds)
{
    entities.for_each<TransformComponent, SpinComponent>(
        [&](Entity, TransformComponent& transform, const SpinComponent& spin) {
            transform.rotation = math::Quaternion::FromAxisAngle(spin.axis, seconds * spin.speed);
        });
}

// Orbit: moves the entities on their circle. A light or an emitter on the same entity follows it.
static void orbitSystem(EntityManager& entities, float seconds)
{
    entities.for_each<TransformComponent, OrbitComponent>(
        [&](Entity, TransformComponent& transform, const OrbitComponent& orbit) {
            const float angle = seconds * orbit.speed;
            transform.position = math::Vector3<float>(
                orbit.center.x + orbit.radius * std::cos(angle),
                orbit.center.y,
                orbit.center.z + orbit.radius * std::sin(angle));
        });
}

// Light sweep: the light stays in place, only where it points at changes.
static void lightSweepSystem(EntityManager& entities, float seconds)
{
    entities.for_each<TransformComponent, LightComponent, LightSweepComponent>(
        [&](Entity, const TransformComponent& transform, LightComponent& light, const LightSweepComponent& sweep) {
            const math::Vector3<float> target(
                sweep.center.x + sweep.amplitude * std::sin(seconds * sweep.speed),
                sweep.center.y,
                sweep.center.z);
            light.light.direction = (target - transform.position).Normalized();
        });
}

// Particle simulation step: spawn, age, move.
static void particleSystem(EntityManager& entities, float deltaTime)
{
    entities.for_each<TransformComponent, ParticleComponent>(
        [&](Entity, const TransformComponent& transform, ParticleComponent& particles) {
            // The emitter sits on its entity, wherever it is
            particles.emitter.position = transform.position;

            if (particles.emitting) particle::emit(particles.emitter, particles.system, deltaTime);
            particle::update(particles.system, deltaTime);
        });
}

// The order matters: the orbit moves an emitter before the particle system spawns from it.
void systems::update(EntityManager& entities, float seconds, float deltaTime)
{
    spinSystem(entities, seconds);
    orbitSystem(entities, seconds);
    lightSweepSystem(entities, seconds);
    particleSystem(entities, deltaTime);
}
