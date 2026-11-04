#pragma once

#include <string>

#include <SDL3/SDL_keycode.h>

#include "Light.h"
#include "Mat4.h"
#include "Material.h"
#include "Mesh.h"
#include "Particle.h"
#include "Quaternion.h"
#include "Vec3.h"

// Components are plain data: what an entity IS. What it DOES lives in the systems (Systems.h).
// A new component type must also get an id in ComponentIDs (below) and be registered in register_components (Scene.cpp).

// The componentID of each component type: what register_component returned for it.
// -1 until it is registered, which the asserts of the EntityManager catch.
struct ComponentIDs
{
	int name = -1;
	int transform = -1;
	int mesh = -1;
	int material = -1;
	int camera = -1;
	int light = -1;
	int particle = -1;
	int spin = -1;
	int orbit = -1;
	int lightSweep = -1;
	int lightToggle = -1;
};


// ---- Identity and placement ----

// What the entity is called: on the HUD, and when debugging.
struct NameComponent
{
	std::string name;
};

// Where an entity is in the world. Every system reads its position from here.
struct TransformComponent
{
	math::Vector3<float> position;
	math::Quaternion rotation;
	math::Vector3<float> scale{ 1.0f, 1.0f, 1.0f };

	math::Mat4<float> model_matrix() const
	{
		return math::Mat4<float>::trs(position, rotation, scale);
	}
};


// ---- What gets drawn ----
// An entity is drawn as a mesh once it has a Transform, a Mesh and a Material.

// The mesh and the material are shared assets: several entities can point to the same one.
// They belong to the Assets struct (Scene.h), which must outlive the entities.
struct MeshComponent
{
	const Mesh* mesh = nullptr;
};

struct MaterialComponent
{
	const Material* material = nullptr;
};

// The point of view of the scene, and how it shows the meshes.
// Its position comes from the entity's TransformComponent. Only the first camera is used.
struct CameraComponent
{
	float fovY = 1.0f; // vertical field of view, in radians
	float nearPlane = 0.1f;
	float farPlane = 100.0f;
	math::Vector3<float> target;
	math::Vector3<float> up{ 0.0f, 1.0f, 0.0f };

	// Key T: makes every mesh see-through (alpha blending, no depth writes).
	// A material can also be transparent on its own with "d" below 1 in the .mtl:
	// T only caps every opacity at transparentOpacity.
	bool transparencyEnabled = false;
	float transparentOpacity = 0.42f;
};

// Light::position is ignored here: point and spot lights are placed by the entity's TransformComponent.
struct LightComponent
{
	Light light;
	bool enabled = true;
};

// Careful with the name: a ParticleSystem is the pool of living particles, not a system in the ECS sense.
// The pool holds the particles alive, the emitter spawns new ones at the entity's position.
struct ParticleComponent
{
	ParticleSystem system;
	ParticleEmitter emitter;
	bool emitting = true; // key P. The particles already alive finish their life.
};


// ---- Behaviours: adding one of these to an entity is enough, a system in Systems.cpp does the rest ----

// Turns the entity around an axis, at speed radians per second.
struct SpinComponent
{
	math::Vector3<float> axis{ 0.0f, 1.0f, 0.0f };
	float speed = 1.0f;
};

// Moves the entity on a horizontal circle around center, at speed radians per second.
struct OrbitComponent
{
	math::Vector3<float> center;
	float radius = 1.0f;
	float speed = 1.0f;
};

// Aims the entity's light at a target going left and right on the X axis around center.
struct LightSweepComponent
{
	math::Vector3<float> center;
	float amplitude = 1.0f;
	float speed = 1.0f;
};

// The key that turns the entity's light on and off.
struct LightToggleComponent
{
	SDL_Keycode key = SDLK_UNKNOWN;
};
