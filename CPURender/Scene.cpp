#include "Scene.h"

#include <array>
#include <cmath>
#include <iostream>
#include <numbers>

// Size of the pool of each particle emitter.
constexpr size_t MAX_PARTICLES = 20000;

bool scene::load_assets(Assets& outAssets)
{
    // Load the mesh and its texture once, before the loop (chapters 6 and 9)
    if (!mesh::load_obj("assets/spot.obj", outAssets.spot)) return false;
    std::cout << "Loaded spot.obj:\t" << outAssets.spot.vertices.size() << " vertices, " << outAssets.spot.uvs.size() << " uvs, " << outAssets.spot.faces.size() << " faces\n";

    if (!texture::load_png("assets/spot_texture.png", outAssets.spotTexture)) return false;
    std::cout << "Loaded spot_texture.png:\t" << outAssets.spotTexture.width << "x" << outAssets.spotTexture.height << " texels\n";

    // Week 2: one material per cow in the shader demo
    if (!material::load_mtl("assets/spot.mtl", outAssets.spotMaterials)) return false;
    std::cout << "Loaded spot.mtl:\t" << outAssets.spotMaterials.size() << " materials\n";

    return true;
}

// Every component type must be registered once, before any entity uses it.
static ComponentIDs register_components(EntityManager& em)
{
    ComponentIDs ids;
    ids.name = em.register_component<NameComponent>();
    ids.transform = em.register_component<TransformComponent>();
    ids.mesh = em.register_component<MeshComponent>();
    ids.material = em.register_component<MaterialComponent>();
    ids.camera = em.register_component<CameraComponent>();
    ids.light = em.register_component<LightComponent>();
    ids.particle = em.register_component<ParticleComponent>();
    ids.spin = em.register_component<SpinComponent>();
    ids.orbit = em.register_component<OrbitComponent>();
    ids.lightSweep = em.register_component<LightSweepComponent>();
    ids.lightToggle = em.register_component<LightToggleComponent>();
    ids.canvas = em.register_component<CanvasComponent>();
    ids.image = em.register_component<ImageComponent>();
    ids.text = em.register_component<TextComponent>();
    return ids;
}

// Reserves the pool on the stored copy: copying a vector doesn't keep its reserved memory.
static void add_particles(EntityManager& em, const ComponentIDs& ids, int entity, ParticleComponent& particles)
{
    em.add_component(entity, ids.particle, &particles);
    particle::create(static_cast<ParticleComponent*>(em.get_component(entity, ids.particle))->system, MAX_PARTICLES);
}

// Camera pulled back to see the 4 cows.
static void create_camera(EntityManager& em, const ComponentIDs& ids)
{
    const int entity = em.create_entity();

    NameComponent name{ "camera" };
    em.add_component(entity, ids.name, &name);

    TransformComponent transform;
    transform.transformMatrix.set_position(math::Vector3<float>(0.0f, 0.0f, 4.2f));
    em.add_component(entity, ids.transform, &transform);

    CameraComponent camera;
    camera.fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    em.add_component(entity, ids.camera, &camera);
}

// White sun, same direction as the light of chapters 10 to 12
static void create_sun(EntityManager& em, const ComponentIDs& ids)
{
    const int entity = em.create_entity();

    NameComponent name{ "sun" };
    em.add_component(entity, ids.name, &name);

    TransformComponent transform;
    em.add_component(entity, ids.transform, &transform);

    LightToggleComponent toggle{ SDLK_1 };
    em.add_component(entity, ids.lightToggle, &toggle);

    LightComponent sun;
    sun.light.type = LightType::Directional;
    sun.light.direction = math::Vector3<float>(-0.4f, -0.8f, -0.45f).normalized();
    sun.light.intensity = 0.8f;
    em.add_component(entity, ids.light, &sun);
}

// Orange bulb circling around the scene. One entity with both a light and an emitter:
// the flame sits on the bulb, wherever it is in its orbit.
static void create_bulb(EntityManager& em, const ComponentIDs& ids)
{
    const int entity = em.create_entity();

    NameComponent name{ "bulb" };
    em.add_component(entity, ids.name, &name);

    TransformComponent transform;
    em.add_component(entity, ids.transform, &transform);

    LightToggleComponent toggle{ SDLK_2 };
    em.add_component(entity, ids.lightToggle, &toggle);

    OrbitComponent orbit;
    orbit.center = math::Vector3<float>(0.0f, 0.5f, 0.0f);
    orbit.radius = 3.0f;
    orbit.speed = 1.0f;
    em.add_component(entity, ids.orbit, &orbit);

    LightComponent bulb;
    bulb.light.type = LightType::Point;
    bulb.light.color = math::Vector3<float>(1.0f, 0.55f, 0.2f);
    bulb.light.intensity = 5.0f;
    em.add_component(entity, ids.light, &bulb);

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

    add_particles(em, ids, entity, particles);
}

// Bluish spotlight above the camera, sweeping the scene from left to right
static void create_spot(EntityManager& em, const ComponentIDs& ids)
{
    const int entity = em.create_entity();

    NameComponent name{ "spot" };
    em.add_component(entity, ids.name, &name);

    LightToggleComponent toggle{ SDLK_3 };
    em.add_component(entity, ids.lightToggle, &toggle);

    TransformComponent transform;
    transform.transformMatrix.set_position(math::Vector3<float>(0.0f, 2.5f, 4.0f));
    em.add_component(entity, ids.transform, &transform);

    LightSweepComponent sweep;
    sweep.center = math::Vector3<float>(0.0f, -0.1f, 0.0f);
    sweep.amplitude = 2.0f;
    sweep.speed = 0.8f;
    em.add_component(entity, ids.lightSweep, &sweep);

    LightComponent spot;
    spot.light.type = LightType::Spot;
    spot.light.color = math::Vector3<float>(0.5f, 0.75f, 1.0f);
    spot.light.intensity = 20.0f;
    spot.light.innerConeCos = std::cos(8.0f * std::numbers::pi_v<float> / 180.0f);
    spot.light.outerConeCos = std::cos(14.0f * std::numbers::pi_v<float> / 180.0f);
    em.add_component(entity, ids.light, &spot);
}

// The same cow drawn once per material of the .mtl, on a 2x2 grid:
// top row unlit then Lambert, bottom row textured Blinn-Phong then untextured gold.
// One grid cell per material, the cows all spin together.
static void create_cows(EntityManager& em, const ComponentIDs& ids, const Assets& assets)
{
    const std::array<math::Vector3<float>, 4> slots = { {
        { -1.1f, 0.85f, 0.0f }, { 1.1f, 0.85f, 0.0f },
        { -1.1f, -1.05f, 0.0f }, { 1.1f, -1.05f, 0.0f } } };

    for (size_t i = 0; i < assets.spotMaterials.size() && i < slots.size(); i++)
    {
        const int entity = em.create_entity();

        NameComponent name{ "cow " + assets.spotMaterials[i].name };
        em.add_component(entity, ids.name, &name);

        TransformComponent transform;
        transform.transformMatrix = math::Mat4<float>::trs(slots[i], math::Quaternion(), math::Vector3<float>(0.7f, 0.7f, 0.7f));
        em.add_component(entity, ids.transform, &transform);

        MeshComponent mesh;
        mesh.mesh = &assets.spot;
        em.add_component(entity, ids.mesh, &mesh);

        MaterialComponent material;
        material.material = &assets.spotMaterials[i];
        em.add_component(entity, ids.material, &material);

        SpinComponent spin;
        spin.speed = 0.7f;
        em.add_component(entity, ids.spin, &spin);
    }
}

// Fountain: solid blue droplets thrown up, pulled down by the gravity,
// bouncing on a floor just below the bottom cows and on the walls of a box around them.
static void create_fountain(EntityManager& em, const ComponentIDs& ids)
{
    const int entity = em.create_entity();

    NameComponent name{ "fountain" };
    em.add_component(entity, ids.name, &name);

    TransformComponent transform;
    transform.transformMatrix.set_position(math::Vector3<float>(0.0f, -1.8f, 0.0f));
    em.add_component(entity, ids.transform, &transform);

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

    add_particles(em, ids, entity, particles);
}

// The creation order sets the entity ids, and so the order the lights and the particles are processed in.
ComponentIDs scene::create(EntityManager& em, const Assets& assets)
{
    const ComponentIDs ids = register_components(em);

    create_camera(em, ids);
    create_sun(em, ids);
    create_bulb(em, ids);
    create_spot(em, ids);
    create_cows(em, ids, assets);
    create_fountain(em, ids);

    return ids;
}
