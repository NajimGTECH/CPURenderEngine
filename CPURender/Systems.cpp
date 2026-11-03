#include "Systems.h"

#include <cmath>

// The input and simulation systems. Drawing is in RenderSystem.cpp, the on-screen text in HudSystem.cpp.

// ---- Input ----

void systems::handleKey(EntityManager& em, const ComponentIDs& ids, SDL_Keycode key)
{
    // T: switches the camera between opaque and see-through meshes.
    if (key == SDLK_T)
    {
        for (int entity : em.FetchAllEntities(ids.camera))
        {
            CameraComponent& camera = *static_cast<CameraComponent*>(em.GetComponent(entity, ids.camera));
            camera.transparencyEnabled = !camera.transparencyEnabled;
        }
    }

    // P: stops / restarts the emitters. The particles already alive finish their life.
    if (key == SDLK_P)
    {
        for (int entity : em.FetchAllEntities(ids.particle))
        {
            ParticleComponent& particles = *static_cast<ParticleComponent*>(em.GetComponent(entity, ids.particle));
            particles.emitting = !particles.emitting;
        }
    }

    // Each light can have its own key, to see what it brings.
    for (int entity : em.FetchAllEntities(ids.light, ids.lightToggle))
    {
        LightComponent& light = *static_cast<LightComponent*>(em.GetComponent(entity, ids.light));
        const LightToggleComponent& toggle = *static_cast<LightToggleComponent*>(em.GetComponent(entity, ids.lightToggle));
        if (toggle.key == key) light.enabled = !light.enabled;
    }
}


// ---- Simulation ----

// Spin: the rotation only depends on the time since the start.
void systems::spinSystem(EntityManager& em, const ComponentIDs& ids, float seconds)
{
    for (int entity : em.FetchAllEntities(ids.transform, ids.spin))
    {
        TransformComponent& transform = *static_cast<TransformComponent*>(em.GetComponent(entity, ids.transform));
        const SpinComponent& spin = *static_cast<SpinComponent*>(em.GetComponent(entity, ids.spin));
        transform.rotation = math::Quaternion::FromAxisAngle(spin.axis, seconds * spin.speed);
    }
}

// Orbit: moves the entities on their circle. A light or an emitter on the same entity follows it.
void systems::orbitSystem(EntityManager& em, const ComponentIDs& ids, float seconds)
{
    for (int entity : em.FetchAllEntities(ids.transform, ids.orbit))
    {
        TransformComponent& transform = *static_cast<TransformComponent*>(em.GetComponent(entity, ids.transform));
        const OrbitComponent& orbit = *static_cast<OrbitComponent*>(em.GetComponent(entity, ids.orbit));

        const float angle = seconds * orbit.speed;
        transform.position = math::Vector3<float>(
            orbit.center.x + orbit.radius * std::cos(angle),
            orbit.center.y,
            orbit.center.z + orbit.radius * std::sin(angle));
    }
}

// Light sweep: the light stays in place, only where it points at changes.
void systems::lightSweepSystem(EntityManager& em, const ComponentIDs& ids, float seconds)
{
    for (int entity : em.FetchAllEntities(ids.transform, ids.light, ids.lightSweep))
    {
        const TransformComponent& transform = *static_cast<TransformComponent*>(em.GetComponent(entity, ids.transform));
        LightComponent& light = *static_cast<LightComponent*>(em.GetComponent(entity, ids.light));
        const LightSweepComponent& sweep = *static_cast<LightSweepComponent*>(em.GetComponent(entity, ids.lightSweep));

        const math::Vector3<float> target(
            sweep.center.x + sweep.amplitude * std::sin(seconds * sweep.speed),
            sweep.center.y,
            sweep.center.z);
        light.light.direction = (target - transform.position).Normalized();
    }
}

// Particle simulation step: spawn, age, move.
void systems::particleSystem(EntityManager& em, const ComponentIDs& ids, float deltaTime)
{
    for (int entity : em.FetchAllEntities(ids.transform, ids.particle))
    {
        const TransformComponent& transform = *static_cast<TransformComponent*>(em.GetComponent(entity, ids.transform));
        ParticleComponent& particles = *static_cast<ParticleComponent*>(em.GetComponent(entity, ids.particle));

        // The emitter sits on its entity, wherever it is
        particles.emitter.position = transform.position;

        if (particles.emitting) particle::emit(particles.emitter, particles.system, deltaTime);
        particle::update(particles.system, deltaTime);
    }
}
