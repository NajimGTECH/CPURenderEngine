#include "Systems.h"

#include "Framebuffer.h"
#include "Rasterizer.h"

#include <algorithm>
#include <limits>
#include <vector>

// The bridge between the ECS and the rasterizer: it reads the components,
// works out the draw order, and hands the rasterizer one mesh at a time.
// The rasterizer itself knows nothing about entities.

// Everything needed to draw one mesh entity.
struct Drawable
{
    math::Mat4<float> model;
    const Mesh* mesh = nullptr;
    const Material* material = nullptr;
    float opacity = 1.0f;
    float distanceToCamera = 0.0f; // squared, only used to sort
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
        light.position = transform.position;
        lights.push_back(light);
    }

    return lights;
}

// Every entity with a transform, a mesh and a material.
static std::vector<Drawable> gather_drawables(EntityManager& em, const ComponentIDs& ids, const CameraComponent& camera, const math::Vector3<float>& cameraPosition)
{
    std::vector<Drawable> drawables;

    for (int entity : em.fetch_all_entities(ids.transform, ids.mesh, ids.material))
    {
        const TransformComponent& transform = *static_cast<TransformComponent*>(em.get_component(entity, ids.transform));
        const MeshComponent& mesh = *static_cast<MeshComponent*>(em.get_component(entity, ids.mesh));
        const MaterialComponent& material = *static_cast<MaterialComponent*>(em.get_component(entity, ids.material));

        Drawable drawable;
        drawable.model = transform.model_matrix();
        drawable.mesh = mesh.mesh;
        drawable.material = material.material;
        drawable.opacity = camera.transparencyEnabled
            ? std::min(material.material->opacity, camera.transparentOpacity)
            : material.material->opacity;
        drawable.distanceToCamera = (transform.position - cameraPosition).length_squared();
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

// Render system: draws the opaque meshes, then the particles, then the transparent meshes.
void systems::render(EntityManager& em, const ComponentIDs& ids)
{
    const std::vector<int> cameras = em.fetch_all_entities(ids.transform, ids.camera);
    if (cameras.empty()) return;

    const int cameraEntity = cameras[0];
    const CameraComponent& camera = *static_cast<CameraComponent*>(em.get_component(cameraEntity, ids.camera));

    // The shader needs the camera position for the highlights.
    const math::Vector3<float> cameraPosition = static_cast<TransformComponent*>(em.get_component(cameraEntity, ids.transform))->position;
    const math::Mat4<float> viewProjection = compute_view_projection(cameraPosition, camera);

    const std::vector<Light> lights = gather_lights(em, ids);

    std::vector<Drawable> drawables = gather_drawables(em, ids, camera, cameraPosition);
    sort_for_drawing(drawables);

    // Every mesh shares the same depth buffer: clear it only once.
    depthBuffer.fill(std::numeric_limits<float>::max());

    // Opaque pass: fills the depth buffer
    for (const Drawable& drawable : drawables)
    {
        if (drawable.opacity < 1.0f) continue;
        rasterizer::draw_mesh(*drawable.mesh, drawable.model, viewProjection, *drawable.material, lights, cameraPosition, drawable.opacity);
    }

    // Particle pass: after the opaque meshes so they can hide the particles behind them,
    // before the transparent ones so the particles show through them.
    for (int entity : em.fetch_all_entities(ids.particle))
    {
        const ParticleComponent& particles = *static_cast<ParticleComponent*>(em.get_component(entity, ids.particle));
        rasterizer::draw_particles(particles.system, viewProjection);
    }

    // Transparent pass: already sorted from back to front
    for (const Drawable& drawable : drawables)
    {
        if (drawable.opacity >= 1.0f) continue;
        rasterizer::draw_mesh(*drawable.mesh, drawable.model, viewProjection, *drawable.material, lights, cameraPosition, drawable.opacity);
    }
}
