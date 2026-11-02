#include "Scene.h"

#include <cmath>
#include <iostream>
#include <iterator>
#include <numbers>

// Size of the pool of each particle emitter.
constexpr size_t MAX_PARTICLES = 20000;

bool scene::loadAssets(Assets& outAssets)
{
    // Load the mesh and its texture once, before the loop (chapters 6 and 9)
    if (!mesh::loadObj("assets/spot.obj", outAssets.spot)) return false;
    std::cout << "Loaded spot.obj:\t" << outAssets.spot.vertices.size() << " vertices, " << outAssets.spot.uvs.size() << " uvs, " << outAssets.spot.faces.size() << " faces\n";

    if (!texture::loadPng("assets/spot_texture.png", outAssets.spotTexture)) return false;
    std::cout << "Loaded spot_texture.png:\t" << outAssets.spotTexture.width << "x" << outAssets.spotTexture.height << " texels\n";

    // Week 2: one material per cow in the shader demo
    if (!material::loadMtl("assets/spot.mtl", outAssets.spotMaterials)) return false;
    std::cout << "Loaded spot.mtl:\t" << outAssets.spotMaterials.size() << " materials\n";

    return true;
}

// Every component type must be registered once, before any entity uses it.
static void registerComponents(EntityManager& entities)
{
    entities.add_component_type<NameComponent>();
    entities.add_component_type<TransformComponent>();
    entities.add_component_type<MeshComponent>();
    entities.add_component_type<MaterialComponent>();
    entities.add_component_type<CameraComponent>();
    entities.add_component_type<LightComponent>();
    entities.add_component_type<ParticleComponent>();
    entities.add_component_type<SpinComponent>();
    entities.add_component_type<OrbitComponent>();
    entities.add_component_type<LightSweepComponent>();
    entities.add_component_type<LightToggleComponent>();
}

// Reserves the pool on the stored copy: copying a vector doesn't keep its reserved memory.
static void addParticles(EntityManager& entities, Entity entity, const ParticleComponent& particles)
{
    particle::create(entities.add_component(entity, particles).system, MAX_PARTICLES);
}

// Camera pulled back to see the 4 cows.
static void createCamera(EntityManager& entities)
{
    const Entity entity = entities.create_entity();
    entities.add_component(entity, NameComponent{ "camera" });

    TransformComponent transform;
    transform.position = math::Vector3<float>(0.0f, 0.0f, 4.2f);
    entities.add_component(entity, transform);

    CameraComponent camera;
    camera.fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    entities.add_component(entity, camera);
}

// White sun, same direction as the light of chapters 10 to 12
static void createSun(EntityManager& entities)
{
    const Entity entity = entities.create_entity();
    entities.add_component(entity, NameComponent{ "sun" });
    entities.add_component(entity, TransformComponent{});
    entities.add_component(entity, LightToggleComponent{ SDLK_1 });

    LightComponent sun;
    sun.light.type = LightType::Directional;
    sun.light.direction = math::Vector3<float>(-0.4f, -0.8f, -0.45f).Normalized();
    sun.light.intensity = 0.8f;
    entities.add_component(entity, sun);
}

// Orange bulb circling around the scene. One entity with both a light and an emitter:
// the flame sits on the bulb, wherever it is in its orbit.
static void createBulb(EntityManager& entities)
{
    const Entity entity = entities.create_entity();
    entities.add_component(entity, NameComponent{ "bulb" });
    entities.add_component(entity, TransformComponent{});
    entities.add_component(entity, LightToggleComponent{ SDLK_2 });

    OrbitComponent orbit;
    orbit.center = math::Vector3<float>(0.0f, 0.5f, 0.0f);
    orbit.radius = 3.0f;
    orbit.speed = 1.0f;
    entities.add_component(entity, orbit);

    LightComponent bulb;
    bulb.light.type = LightType::Point;
    bulb.light.color = math::Vector3<float>(1.0f, 0.55f, 0.2f);
    bulb.light.intensity = 5.0f;
    entities.add_component(entity, bulb);

    // Fire: glowing red to orange particles. The gravity points UP (hot air rises)
    // and a strong drag quickly kills the random launch speed, so the flame narrows as it climbs.
    ParticleComponent particles;
    particles.system.physics.gravity = math::Vector3<float>(0.0f, 0.6f, 0.0f);
    particles.system.physics.drag = 1.5f;

    ParticleEmitter& fire = particles.emitter;
    fire.blend = ParticleBlend::Intensity;
    fire.rate = 4000.0f;
    fire.spawnRadius = 0.06f;
    fire.direction = math::Vector3<float>(0.0f, 1.0f, 0.0f);
    fire.colorMin = math::Vector3<float>(1.0f, 0.1f, 0.0f);
    fire.colorMax = math::Vector3<float>(1.0f, 0.6f, 0.1f);
    fire.durationMin = 0.4f;
    fire.durationMax = 1.2f;
    fire.speedMin = 0.3f;
    fire.speedMax = 0.8f;
    fire.spread = 0.6f;
    fire.seed = 1234u;

    addParticles(entities, entity, particles);
}

// Bluish spotlight above the camera, sweeping the scene from left to right
static void createSpot(EntityManager& entities)
{
    const Entity entity = entities.create_entity();
    entities.add_component(entity, NameComponent{ "spot" });
    entities.add_component(entity, LightToggleComponent{ SDLK_3 });

    TransformComponent transform;
    transform.position = math::Vector3<float>(0.0f, 2.5f, 4.0f);
    entities.add_component(entity, transform);

    LightSweepComponent sweep;
    sweep.center = math::Vector3<float>(0.0f, -0.1f, 0.0f);
    sweep.amplitude = 2.0f;
    sweep.speed = 0.8f;
    entities.add_component(entity, sweep);

    LightComponent spot;
    spot.light.type = LightType::Spot;
    spot.light.color = math::Vector3<float>(0.5f, 0.75f, 1.0f);
    spot.light.intensity = 20.0f;
    spot.light.innerConeCos = std::cos(8.0f * std::numbers::pi_v<float> / 180.0f);
    spot.light.outerConeCos = std::cos(14.0f * std::numbers::pi_v<float> / 180.0f);
    entities.add_component(entity, spot);
}

// The same cow drawn once per material of the .mtl, on a 2x2 grid:
// top row unlit then Lambert, bottom row textured Blinn-Phong then untextured gold.
// One grid cell per material, the cows all spin together.
static void createCows(EntityManager& entities, const Assets& assets)
{
    const math::Vector3<float> slots[] = {
        { -1.1f, 0.85f, 0.0f }, { 1.1f, 0.85f, 0.0f },
        { -1.1f, -1.05f, 0.0f }, { 1.1f, -1.05f, 0.0f } };

    for (size_t i = 0; i < assets.spotMaterials.size() && i < std::size(slots); i++)
    {
        const Entity entity = entities.create_entity();
        entities.add_component(entity, NameComponent{ "cow " + assets.spotMaterials[i].name });

        TransformComponent transform;
        transform.position = slots[i];
        transform.scale = math::Vector3<float>(0.7f, 0.7f, 0.7f);
        entities.add_component(entity, transform);

        MeshComponent mesh;
        mesh.mesh = &assets.spot;
        entities.add_component(entity, mesh);

        MaterialComponent material;
        material.material = &assets.spotMaterials[i];
        entities.add_component(entity, material);

        SpinComponent spin;
        spin.speed = 0.7f;
        entities.add_component(entity, spin);
    }
}

// Fountain: solid blue droplets thrown up, pulled down by the gravity,
// bouncing on a floor just below the bottom cows and on the walls of a box around them.
static void createFountain(EntityManager& entities)
{
    const Entity entity = entities.create_entity();
    entities.add_component(entity, NameComponent{ "fountain" });

    TransformComponent transform;
    transform.position = math::Vector3<float>(0.0f, -1.8f, 0.0f);
    entities.add_component(entity, transform);

    ParticleComponent particles;
    particles.system.physics.gravity = math::Vector3<float>(0.0f, -4.0f, 0.0f);
    particles.system.physics.drag = 0.2f;
    particles.system.physics.useBounds = true;
    particles.system.physics.boundsMin = math::Vector3<float>(-2.5f, -1.8f, -1.5f);
    particles.system.physics.boundsMax = math::Vector3<float>(2.5f, 3.0f, 1.5f);
    particles.system.physics.bounce = 0.3f;

    ParticleEmitter& fountain = particles.emitter;
    fountain.blend = ParticleBlend::Opaque;
    fountain.rate = 1500.0f;
    fountain.spawnRadius = 0.03f;
    fountain.direction = math::Vector3<float>(0.0f, 1.0f, 0.0f);
    fountain.colorMin = math::Vector3<float>(0.1f, 0.3f, 1.0f);
    fountain.colorMax = math::Vector3<float>(0.4f, 0.8f, 1.0f);
    fountain.durationMin = 1.5f;
    fountain.durationMax = 2.5f;
    fountain.speedMin = 3.0f;
    fountain.speedMax = 3.5f;
    fountain.spread = 0.1f;
    fountain.seed = 5678u;

    addParticles(entities, entity, particles);
}

// The creation order sets the entity ids, and so the order the lights and the particles are processed in.
void scene::create(EntityManager& entities, const Assets& assets)
{
    registerComponents(entities);

    createCamera(entities);
    createSun(entities);
    createBulb(entities);
    createSpot(entities);
    createCows(entities, assets);
    createFountain(entities);
}
