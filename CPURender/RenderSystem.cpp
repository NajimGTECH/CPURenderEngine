#include "Systems.h"

#include "Framebuffer.h"
#include "Rasterizer.h"
#include "Tiles.h"

#include <algorithm>
#include <limits>
#include <vector>

// The bridge between the ECS and the rasterizer: it reads the components,
// works out the draw order, and hands the rasterizer one mesh at a time.
// The rasterizer itself knows nothing about entities.
//
// The screen is drawn in tiles, one per thread (Tiles.h). The entities are read first, on the main thread,
// into a RenderList of plain data; then every tile draws that list. The threads never touch the EntityManager.

// Everything needed to draw one mesh entity.
struct Drawable
{
    math::Mat4<float> model;
    const Mesh* mesh = nullptr;
    const Material* material = nullptr;
    float opacity = 1.0f;
    float distanceToCamera = 0.0f; // squared, only used to sort
    BoundingBox screenBox;         // the pixels the mesh can cover, to skip the tiles it doesn't touch
};

// Everything a tile needs to draw the frame. Filled once per frame, then only read by the threads.
struct RenderList
{
    math::Mat4<float> viewProjection;
    math::Vector3<float> cameraPosition;
    std::vector<Light> lights;
    std::vector<Drawable> drawables; // opaque first, then transparent from back to front
    std::vector<const ParticleSystem*> particleSystems;
};

static math::Mat4<float> compute_view_projection(const math::Vector3<float>& cameraPosition, const CameraComponent& camera)
{
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const math::Mat4<float> projection = math::Mat4<float>::perspective(camera.fovY, aspect, camera.nearPlane, camera.farPlane);
    const math::Mat4<float> view = math::Mat4<float>::look_at(cameraPosition, camera.target, camera.up);

    // Row vectors (p * M): view, then projection.
    return view * projection;
}

// The lights of the scene, gathered every frame because some of them move.
static std::vector<Light> gather_lights(EntityManager& em, const ComponentIDs& ids)
{
    std::vector<Light> lights;

    for (int entity : em.fetch_all_entities(ids.transform, ids.light))
    {
        const TransformComponent& transform = *static_cast<TransformComponent*>(em.get_component(entity, ids.transform));
        const LightComponent& component = *static_cast<LightComponent*>(em.get_component(entity, ids.light));
        if (!component.enabled) continue;

        Light light = component.light;
        light.position = transform.transformMatrix.extract_position();
        lights.push_back(light);
    }

    return lights;
}

// Every entity with a transform, a mesh and a material.
static std::vector<Drawable> gather_drawables(EntityManager& em, const ComponentIDs& ids, const CameraComponent& camera,
    const math::Vector3<float>& cameraPosition, const math::Mat4<float>& viewProjection)
{
    std::vector<Drawable> drawables;

    for (int entity : em.fetch_all_entities(ids.transform, ids.mesh, ids.material))
    {
        const TransformComponent& transform = *static_cast<TransformComponent*>(em.get_component(entity, ids.transform));
        const MeshComponent& mesh = *static_cast<MeshComponent*>(em.get_component(entity, ids.mesh));
        const MaterialComponent& material = *static_cast<MaterialComponent*>(em.get_component(entity, ids.material));

        Drawable drawable;
        drawable.model = transform.transformMatrix;
        drawable.mesh = mesh.mesh;
        drawable.material = material.material;
        drawable.opacity = camera.transparencyEnabled
            ? std::min(material.material->opacity, camera.transparentOpacity)
            : material.material->opacity;
        drawable.distanceToCamera = (transform.transformMatrix.extract_position() - cameraPosition).length_squared();
        drawable.screenBox = rasterizer::screen_box(*mesh.mesh, transform.transformMatrix, viewProjection);
        drawables.push_back(drawable);
    }

    return drawables;
}

// Draw order: every opaque mesh first (they fill the depth buffer),
// then the transparent ones from the farthest to the closest, so each one blends over what is behind it.
static void sort_for_drawing(std::vector<Drawable>& drawables)
{
    std::stable_sort(drawables.begin(), drawables.end(), [](const Drawable& a, const Drawable& b) {
        const bool aTransparent = a.opacity < 1.0f;
        const bool bTransparent = b.opacity < 1.0f;
        if (aTransparent != bTransparent) return !aTransparent;
        return aTransparent && a.distanceToCamera > b.distanceToCamera;
        });
}

// The particle pools of every entity that has some. Pointers: the pools stay in their components.
static std::vector<const ParticleSystem*> gather_particle_systems(EntityManager& em, const ComponentIDs& ids)
{
    std::vector<const ParticleSystem*> systems;

    for (int entity : em.fetch_all_entities(ids.particle))
    {
        const ParticleComponent& particles = *static_cast<ParticleComponent*>(em.get_component(entity, ids.particle));
        systems.push_back(&particles.system);
    }

    return systems;
}

// A mesh whose screen box doesn't overlap the tile can't write any of its pixels: the tile skips it.
static bool touches(const Drawable& drawable, const BoundingBox& tile)
{
    return !rasterizer::is_empty(rasterizer::intersect(drawable.screenBox, tile));
}

// The three passes, restricted to the pixels of one tile. Every tile runs them in the same order,
// so inside a tile everything is drawn in exactly the same order as on the whole screen without threads.
static void draw_tile(const RenderList& list, const BoundingBox& tile)
{
    // Opaque pass: fills the depth buffer
    for (const Drawable& drawable : list.drawables)
    {
        if (drawable.opacity < 1.0f || !touches(drawable, tile)) continue;
        rasterizer::draw_mesh(*drawable.mesh, drawable.model, list.viewProjection, *drawable.material,
            list.lights, list.cameraPosition, drawable.opacity, tile);
    }

    // Particle pass: after the opaque meshes so they can hide the particles behind them,
    // before the transparent ones so the particles show through them.
    for (const ParticleSystem* system : list.particleSystems)
    {
        rasterizer::draw_particles(*system, list.viewProjection, tile);
    }

    // Transparent pass: already sorted from back to front
    for (const Drawable& drawable : list.drawables)
    {
        if (drawable.opacity >= 1.0f || !touches(drawable, tile)) continue;
        rasterizer::draw_mesh(*drawable.mesh, drawable.model, list.viewProjection, *drawable.material,
            list.lights, list.cameraPosition, drawable.opacity, tile);
    }
}

// Render system: draws the opaque meshes, then the particles, then the transparent meshes, in every tile at once.
void systems::render(EntityManager& em, const ComponentIDs& ids)
{
    // The framebuffer never changes size, so neither do the tiles: the grid is made on the first frame only.
    static const std::vector<BoundingBox> grid = tiles::make_grid();

    const std::vector<int> cameras = em.fetch_all_entities(ids.transform, ids.camera);
    if (cameras.empty()) return;

    const int cameraEntity = cameras[0];
    const CameraComponent& camera = *static_cast<CameraComponent*>(em.get_component(cameraEntity, ids.camera));

    // Read everything from the entities now, before the threads start.
    RenderList list;
    // The shader needs the camera position for the highlights.
    list.cameraPosition = static_cast<TransformComponent*>(em.get_component(cameraEntity, ids.transform))->transformMatrix.extract_position();
    list.viewProjection = compute_view_projection(list.cameraPosition, camera);
    list.lights = gather_lights(em, ids);
    list.drawables = gather_drawables(em, ids, camera, list.cameraPosition, list.viewProjection);
    sort_for_drawing(list.drawables);
    list.particleSystems = gather_particle_systems(em, ids);

    // Every mesh shares the same depth buffer: clear it only once, before any tile starts.
    depthBuffer.fill(std::numeric_limits<float>::max());

    // Each tile draws the whole list into its own pixels. Returns when every tile is done.
    tiles::run_in_parallel(grid, [&list](const BoundingBox& tile) { draw_tile(list, tile); });
}
