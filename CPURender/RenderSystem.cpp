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

static math::Mat4<float> computeViewProjection(const math::Vector3<float>& cameraPosition, const CameraComponent& camera)
{
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(camera.fovY, aspect, camera.nearPlane, camera.farPlane);
    const math::Mat4<float> view = math::Mat4<float>::LookAt(cameraPosition, camera.target, camera.up);

    // Row vectors (p * M): view, then projection.
    return view * projection;
}

// The lights of the scene, gathered every frame because some of them move.
static std::vector<Light> gatherLights(EntityManager& entities)
{
    std::vector<Light> lights;

    entities.for_each<TransformComponent, LightComponent>(
        [&](Entity, const TransformComponent& transform, const LightComponent& component) {
            if (!component.enabled) return;

            Light light = component.light;
            light.position = transform.position;
            lights.push_back(light);
        });

    return lights;
}

// Every entity with a transform, a mesh and a material.
static std::vector<Drawable> gatherDrawables(EntityManager& entities, const CameraComponent& camera, const math::Vector3<float>& cameraPosition)
{
    std::vector<Drawable> drawables;

    entities.for_each<TransformComponent, MeshComponent, MaterialComponent>(
        [&](Entity, const TransformComponent& transform, const MeshComponent& mesh, const MaterialComponent& material) {
            Drawable drawable;
            drawable.model = transform.model_matrix();
            drawable.mesh = mesh.mesh;
            drawable.material = material.material;
            drawable.opacity = camera.transparencyEnabled
                ? std::min(material.material->opacity, camera.transparentOpacity)
                : material.material->opacity;
            drawable.distanceToCamera = (transform.position - cameraPosition).LengthSquared();
            drawables.push_back(drawable);
        });

    return drawables;
}

// Draw order: every opaque mesh first (they fill the depth buffer),
// then the transparent ones from the farthest to the closest, so each one blends over what is behind it.
static void sortForDrawing(std::vector<Drawable>& drawables)
{
    std::stable_sort(drawables.begin(), drawables.end(), [](const Drawable& a, const Drawable& b) {
        const bool aTransparent = a.opacity < 1.0f;
        const bool bTransparent = b.opacity < 1.0f;
        if (aTransparent != bTransparent) return !aTransparent;
        return aTransparent && a.distanceToCamera > b.distanceToCamera;
        });
}

// Render system: draws the opaque meshes, then the particles, then the transparent meshes.
void systems::render(EntityManager& entities)
{
    const Entity cameraEntity = entities.find_first<TransformComponent, CameraComponent>();
    if (cameraEntity == NO_ENTITY) return;

    const CameraComponent& camera = entities.get_component<CameraComponent>(cameraEntity);

    // The shader needs the camera position for the highlights.
    const math::Vector3<float> cameraPosition = entities.get_component<TransformComponent>(cameraEntity).position;
    const math::Mat4<float> viewProjection = computeViewProjection(cameraPosition, camera);

    const std::vector<Light> lights = gatherLights(entities);

    std::vector<Drawable> drawables = gatherDrawables(entities, camera, cameraPosition);
    sortForDrawing(drawables);

    // Every mesh shares the same depth buffer: clear it only once.
    std::fill_n(depthBuffer, WIDTH * HEIGHT, std::numeric_limits<float>::max());

    // Opaque pass: fills the depth buffer
    for (const Drawable& drawable : drawables)
    {
        if (drawable.opacity < 1.0f) continue;
        rasterizer::drawMesh(*drawable.mesh, drawable.model, viewProjection, *drawable.material, lights, cameraPosition, drawable.opacity);
    }

    // Particle pass: after the opaque meshes so they can hide the particles behind them,
    // before the transparent ones so the particles show through them.
    entities.for_each<ParticleComponent>([&](Entity, const ParticleComponent& particles) {
        rasterizer::drawParticles(particles.system, viewProjection);
        });

    // Transparent pass: already sorted from back to front
    for (const Drawable& drawable : drawables)
    {
        if (drawable.opacity >= 1.0f) continue;
        rasterizer::drawMesh(*drawable.mesh, drawable.model, viewProjection, *drawable.material, lights, cameraPosition, drawable.opacity);
    }
}
