#include "Systems.h"

#include "Framebuffer.h"
#include "Rasterizer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

// Spin: the rotation only depends on the time since the start.
static void spinSystem(EntityManager& entities, float seconds)
{
    entities.for_each<TransformComponent, SpinComponent>(
        [&](int, TransformComponent& transform, const SpinComponent& spin) {
            transform.rotation = math::Quaternion::FromAxisAngle(spin.axis, seconds * spin.speed);
        });
}

// Orbit: moves the entities on their circle. A light or an emitter on the same entity follows it.
static void orbitSystem(EntityManager& entities, float seconds)
{
    entities.for_each<TransformComponent, OrbitComponent>(
        [&](int, TransformComponent& transform, const OrbitComponent& orbit) {
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
        [&](int, const TransformComponent& transform, LightComponent& light, const LightSweepComponent& sweep) {
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
        [&](int, const TransformComponent& transform, ParticleComponent& particles) {
            // The emitter sits on its entity, wherever it is
            particles.emitter.position = transform.position;

            if (particles.emitting) particle::emit(particles.emitter, particles.system, deltaTime);
            particle::update(particles.system, deltaTime);
        });
}

void systems::update(EntityManager& entities, float seconds, float deltaTime)
{
    spinSystem(entities, seconds);
    orbitSystem(entities, seconds);
    lightSweepSystem(entities, seconds);
    particleSystem(entities, deltaTime);
}

// The lights of the scene, gathered every frame because some of them move.
static std::vector<Light> gatherLights(EntityManager& entities)
{
    std::vector<Light> lights;

    entities.for_each<TransformComponent, LightComponent>(
        [&](int, const TransformComponent& transform, const LightComponent& component) {
            if (!component.enabled) return;

            Light light = component.light;
            light.position = transform.position;
            lights.push_back(light);
        });

    return lights;
}

// Render system: draws the opaque meshes, then the particles, then the transparent meshes.
void systems::render(EntityManager& entities, const RenderSettings& settings)
{
    // The first camera found is the one we look through.
    bool hasCamera = false;
    math::Vector3<float> cameraPosition;
    math::Mat4<float> viewProjection;

    entities.for_each<TransformComponent, CameraComponent>(
        [&](int, const TransformComponent& transform, const CameraComponent& camera) {
            if (hasCamera) return;
            hasCamera = true;

            const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
            const math::Mat4<float> projection = math::Mat4<float>::Perspective(camera.fovY, aspect, camera.nearPlane, camera.farPlane);

            // The shader needs the camera position for the highlights.
            cameraPosition = transform.position;
            const math::Mat4<float> view = math::Mat4<float>::LookAt(cameraPosition, camera.target, camera.up);
            viewProjection = view * projection;
        });

    if (!hasCamera) return;

    const std::vector<Light> lights = gatherLights(entities);

    // Every mesh shares the same depth buffer: clear it only once.
    std::fill_n(depthBuffer, WIDTH * HEIGHT, std::numeric_limits<float>::max());

    struct Drawable
    {
        math::Mat4<float> model;
        const Mesh* mesh = nullptr;
        const Material* material = nullptr;
        float opacity = 1.0f;
        float distanceToCamera = 0.0f; // squared, only used to sort
    };

    std::vector<Drawable> drawables;
    entities.for_each<TransformComponent, MeshComponent, MaterialComponent>(
        [&](int, const TransformComponent& transform, const MeshComponent& mesh, const MaterialComponent& material) {
            Drawable drawable;
            drawable.model = transform.model_matrix();
            drawable.mesh = mesh.mesh;
            drawable.material = material.material;
            drawable.opacity = settings.transparencyEnabled
                ? std::min(material.material->opacity, TRANSPARENT_OPACITY)
                : material.material->opacity;
            drawable.distanceToCamera = (transform.position - cameraPosition).LengthSquared();
            drawables.push_back(drawable);
        });

    // Draw order: every opaque mesh first (they fill the depth buffer),
    // then the transparent ones from the farthest to the closest, so each one blends over what is behind it.
    std::stable_sort(drawables.begin(), drawables.end(), [](const Drawable& a, const Drawable& b) {
        const bool aTransparent = a.opacity < 1.0f;
        const bool bTransparent = b.opacity < 1.0f;
        if (aTransparent != bTransparent) return !aTransparent;
        return aTransparent && a.distanceToCamera > b.distanceToCamera;
        });

    // Opaque pass: fills the depth buffer
    for (const Drawable& drawable : drawables)
    {
        if (drawable.opacity < 1.0f) continue;
        rasterizer::drawMesh(*drawable.mesh, drawable.model, viewProjection, *drawable.material, lights, cameraPosition, drawable.opacity);
    }

    // Particle pass: after the opaque meshes so they can hide the particles behind them,
    // before the transparent ones so the particles show through them.
    entities.for_each<ParticleComponent>([&](int, const ParticleComponent& particles) {
        rasterizer::drawParticles(particles.system, viewProjection);
        });

    // Transparent pass: already sorted from back to front
    for (const Drawable& drawable : drawables)
    {
        if (drawable.opacity >= 1.0f) continue;
        rasterizer::drawMesh(*drawable.mesh, drawable.model, viewProjection, *drawable.material, lights, cameraPosition, drawable.opacity);
    }
}
