#include "Systems.h"

#include <cmath>

// The input and simulation systems. Drawing is in RenderSystem.cpp, the on-screen text in HudSystem.cpp.

// ---- Input ----

void systems::handle_key(EntityManager& em, const ComponentIDs& ids, SDL_Keycode key)
{
    // T: switches the camera between opaque and see-through meshes.
    if (key == SDLK_T)
    {
        for (int entity : em.fetch_all_entities(ids.camera))
        {
            CameraComponent& camera = *static_cast<CameraComponent*>(em.get_component(entity, ids.camera));
            camera.transparencyEnabled = !camera.transparencyEnabled;
        }
    }

    // P: stops / restarts the emitters. The particles already alive finish their life.
    if (key == SDLK_P)
    {
        for (int entity : em.fetch_all_entities(ids.particle))
        {
            ParticleComponent& particles = *static_cast<ParticleComponent*>(em.get_component(entity, ids.particle));
            particles.emitting = !particles.emitting;
        }
    }

    // Each light can have its own key, to see what it brings.
    for (int entity : em.fetch_all_entities(ids.light, ids.lightToggle))
    {
        LightComponent& light = *static_cast<LightComponent*>(em.get_component(entity, ids.light));
        const LightToggleComponent& toggle = *static_cast<LightToggleComponent*>(em.get_component(entity, ids.lightToggle));
        if (toggle.key == key) light.enabled = !light.enabled;
    }
}


// ---- Simulation ----

// Spin: the rotation only depends on the time since the start.
void systems::spin_system(EntityManager& em, const ComponentIDs& ids, float seconds)
{
    for (int entity : em.fetch_all_entities(ids.transform, ids.spin))
    {
        TransformComponent& transform = *static_cast<TransformComponent*>(em.get_component(entity, ids.transform));
        const SpinComponent& spin = *static_cast<SpinComponent*>(em.get_component(entity, ids.spin));
        transform.rotation = math::Quaternion::from_axis_angle(spin.axis, seconds * spin.speed);
    }
}

// Orbit: moves the entities on their circle. A light or an emitter on the same entity follows it.
void systems::orbit_system(EntityManager& em, const ComponentIDs& ids, float seconds)
{
    for (int entity : em.fetch_all_entities(ids.transform, ids.orbit))
    {
        TransformComponent& transform = *static_cast<TransformComponent*>(em.get_component(entity, ids.transform));
        const OrbitComponent& orbit = *static_cast<OrbitComponent*>(em.get_component(entity, ids.orbit));

        const float angle = seconds * orbit.speed;
        transform.position = math::Vector3<float>(
            orbit.center.x + orbit.radius * std::cos(angle),
            orbit.center.y,
            orbit.center.z + orbit.radius * std::sin(angle));
    }
}

// Light sweep: the light stays in place, only where it points at changes.
void systems::light_sweep_system(EntityManager& em, const ComponentIDs& ids, float seconds)
{
    for (int entity : em.fetch_all_entities(ids.transform, ids.light, ids.lightSweep))
    {
        const TransformComponent& transform = *static_cast<TransformComponent*>(em.get_component(entity, ids.transform));
        LightComponent& light = *static_cast<LightComponent*>(em.get_component(entity, ids.light));
        const LightSweepComponent& sweep = *static_cast<LightSweepComponent*>(em.get_component(entity, ids.lightSweep));

        const math::Vector3<float> target(
            sweep.center.x + sweep.amplitude * std::sin(seconds * sweep.speed),
            sweep.center.y,
            sweep.center.z);
        light.light.direction = (target - transform.position).normalized();
    }
}

// Particle simulation step: spawn, age, move.
void systems::particle_system(EntityManager& em, const ComponentIDs& ids, float deltaTime)
{
    for (int entity : em.fetch_all_entities(ids.transform, ids.particle))
    {
        const TransformComponent& transform = *static_cast<TransformComponent*>(em.get_component(entity, ids.transform));
        ParticleComponent& particles = *static_cast<ParticleComponent*>(em.get_component(entity, ids.particle));

        // The emitter sits on its entity, wherever it is
        particles.emitter.position = transform.position;

        if (particles.emitting) particle::emit(particles.emitter, particles.system, deltaTime);
        particle::update(particles.system, deltaTime);
    }
}
