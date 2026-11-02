#pragma once

#include <iostream>
#include <string>

#include "Light.h"
#include "Mat4.h"
#include "Material.h"
#include "Mesh.h"
#include "Particle.h"
#include "Quaternion.h"
#include "Vec3.h"

// Where an entity is in the world. Every system reads its position from here.
struct TransformComponent
{
	math::Vector3<float> position;
	math::Quaternion rotation;
	math::Vector3<float> scale{ 1.0f, 1.0f, 1.0f };

	math::Mat4<float> model_matrix() const
	{
		return math::Mat4<float>::TRS(position, rotation, scale);
	}
};

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

// The point of view of the scene. Its position comes from the entity's TransformComponent.
struct CameraComponent
{
	float fovY = 1.0f; // vertical field of view, in radians
	float nearPlane = 0.1f;
	float farPlane = 100.0f;
	math::Vector3<float> target;
	math::Vector3<float> up{ 0.0f, 1.0f, 0.0f };
};

// Light::position is ignored here: point and spot lights are placed by the entity's TransformComponent.
struct LightComponent
{
	Light light;
	bool enabled = true; // keys 1, 2 and 3
};

// The system holds the particles alive, the emitter spawns new ones at the entity's position.
struct ParticleComponent
{
	ParticleSystem system;
	ParticleEmitter emitter;
	bool emitting = true; // key P. The particles already alive finish their life.
};

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

struct PrintComponent
{
	std::string message = "Hello World!\n";

	void print_message()
	{
		std::cout << message;
	}
};
