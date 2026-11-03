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
static ComponentIDs registerComponents(EntityManager& em)
{
    ComponentIDs ids;
    ids.name = em.RegisterComponent<NameComponent>();
    ids.transform = em.RegisterComponent<TransformComponent>();
    ids.mesh = em.RegisterComponent<MeshComponent>();
    ids.material = em.RegisterComponent<MaterialComponent>();
    ids.camera = em.RegisterComponent<CameraComponent>();
    ids.light = em.RegisterComponent<LightComponent>();
    ids.particle = em.RegisterComponent<ParticleComponent>();
    ids.spin = em.RegisterComponent<SpinComponent>();
    ids.orbit = em.RegisterComponent<OrbitComponent>();
    ids.lightSweep = em.RegisterComponent<LightSweepComponent>();
    ids.lightToggle = em.RegisterComponent<LightToggleComponent>();
    return ids;
}

// Reserves the pool on the stored copy: copying a vector doesn't keep its reserved memory.
static void addParticles(EntityManager& em, const ComponentIDs& ids, int entity, ParticleComponent& particles)
{
    em.AddComponent(entity, ids.particle, &particles);
    particle::create(static_cast<ParticleComponent*>(em.GetComponent(entity, ids.particle))->system, MAX_PARTICLES);
}

// Camera pulled back to see the 4 cows.
static void createCamera(EntityManager& em, const ComponentIDs& ids)
{
    const int entity = em.CreateEntity();

    NameComponent name{ "camera" };
    em.AddComponent(entity, ids.name, &name);

    TransformComponent transform;
    transform.position = math::Vector3<float>(0.0f, 0.0f, 4.2f);
    em.AddComponent(entity, ids.transform, &transform);

    CameraComponent camera;
    camera.fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    em.AddComponent(entity, ids.camera, &camera);
}

// White sun, same direction as the light of chapters 10 to 12
static void createSun(EntityManager& em, const ComponentIDs& ids)
{
    const int entity = em.CreateEntity();

    NameComponent name{ "sun" };
    em.AddComponent(entity, ids.name, &name);

    TransformComponent transform;
    em.AddComponent(entity, ids.transform, &transform);

    LightToggleComponent toggle{ SDLK_1 };
    em.AddComponent(entity, ids.lightToggle, &toggle);

    LightComponent sun;
    sun.light.type = LightType::Directional;
    sun.light.direction = math::Vector3<float>(-0.4f, -0.8f, -0.45f).Normalized();
    sun.light.intensity = 0.8f;
    em.AddComponent(entity, ids.light, &sun);
}

// Orange bulb circling around the scene. One entity with both a light and an emitter:
// the flame sits on the bulb, wherever it is in its orbit.
static void createBulb(EntityManager& em, const ComponentIDs& ids)
{
    const int entity = em.CreateEntity();

    NameComponent name{ "bulb" };
    em.AddComponent(entity, ids.name, &name);

    TransformComponent transform;
    em.AddComponent(entity, ids.transform, &transform);

    LightToggleComponent toggle{ SDLK_2 };
    em.AddComponent(entity, ids.lightToggle, &toggle);

    OrbitComponent orbit;
    orbit.center = math::Vector3<float>(0.0f, 0.5f, 0.0f);
    orbit.radius = 3.0f;
    orbit.speed = 1.0f;
    em.AddComponent(entity, ids.orbit, &orbit);

    LightComponent bulb;
    bulb.light.type = LightType::Point;
    bulb.light.color = math::Vector3<float>(1.0f, 0.55f, 0.2f);
    bulb.light.intensity = 5.0f;
    em.AddComponent(entity, ids.light, &bulb);

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

    addParticles(em, ids, entity, particles);
}

// Bluish spotlight above the camera, sweeping the scene from left to right
static void createSpot(EntityManager& em, const ComponentIDs& ids)
{
    const int entity = em.CreateEntity();

    NameComponent name{ "spot" };
    em.AddComponent(entity, ids.name, &name);

    LightToggleComponent toggle{ SDLK_3 };
    em.AddComponent(entity, ids.lightToggle, &toggle);

    TransformComponent transform;
    transform.position = math::Vector3<float>(0.0f, 2.5f, 4.0f);
    em.AddComponent(entity, ids.transform, &transform);

    LightSweepComponent sweep;
    sweep.center = math::Vector3<float>(0.0f, -0.1f, 0.0f);
    sweep.amplitude = 2.0f;
    sweep.speed = 0.8f;
    em.AddComponent(entity, ids.lightSweep, &sweep);

    LightComponent spot;
    spot.light.type = LightType::Spot;
    spot.light.color = math::Vector3<float>(0.5f, 0.75f, 1.0f);
    spot.light.intensity = 20.0f;
    spot.light.innerConeCos = std::cos(8.0f * std::numbers::pi_v<float> / 180.0f);
    spot.light.outerConeCos = std::cos(14.0f * std::numbers::pi_v<float> / 180.0f);
    em.AddComponent(entity, ids.light, &spot);
}

// The same cow drawn once per material of the .mtl, on a 2x2 grid:
// top row unlit then Lambert, bottom row textured Blinn-Phong then untextured gold.
// One grid cell per material, the cows all spin together.
static void createCows(EntityManager& em, const ComponentIDs& ids, const Assets& assets)
{
    const math::Vector3<float> slots[] = {
        { -1.1f, 0.85f, 0.0f }, { 1.1f, 0.85f, 0.0f },
        { -1.1f, -1.05f, 0.0f }, { 1.1f, -1.05f, 0.0f } };

    for (size_t i = 0; i < assets.spotMaterials.size() && i < std::size(slots); i++)
    {
        const int entity = em.CreateEntity();

        NameComponent name{ "cow " + assets.spotMaterials[i].name };
        em.AddComponent(entity, ids.name, &name);

        TransformComponent transform;
        transform.position = slots[i];
        transform.scale = math::Vector3<float>(0.7f, 0.7f, 0.7f);
        em.AddComponent(entity, ids.transform, &transform);

        MeshComponent mesh;
        mesh.mesh = &assets.spot;
        em.AddComponent(entity, ids.mesh, &mesh);

        MaterialComponent material;
        material.material = &assets.spotMaterials[i];
        em.AddComponent(entity, ids.material, &material);

        SpinComponent spin;
        spin.speed = 0.7f;
        em.AddComponent(entity, ids.spin, &spin);
    }
}

// Fountain: solid blue droplets thrown up, pulled down by the gravity,
// bouncing on a floor just below the bottom cows and on the walls of a box around them.
static void createFountain(EntityManager& em, const ComponentIDs& ids)
{
    const int entity = em.CreateEntity();

    NameComponent name{ "fountain" };
    em.AddComponent(entity, ids.name, &name);

    TransformComponent transform;
    transform.position = math::Vector3<float>(0.0f, -1.8f, 0.0f);
    em.AddComponent(entity, ids.transform, &transform);

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

    addParticles(em, ids, entity, particles);
}

// The creation order sets the entity ids, and so the order the lights and the particles are processed in.
ComponentIDs scene::create(EntityManager& em, const Assets& assets)
{
    const ComponentIDs ids = registerComponents(em);

    createCamera(em, ids);
    createSun(em, ids);
    createBulb(em, ids);
    createSpot(em, ids);
    createCows(em, ids, assets);
    createFountain(em, ids);

    return ids;
}
