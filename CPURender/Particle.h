#pragma once

#include <cstdint>
#include <vector>

#include "Vec3.h"

// A pixel particle system, like the one of the school reference engine (cpu-engine).
// Every particle is a single point in 3D: no mesh, no triangle. Each frame it moves (physics),
// gets older, and once projected it lights up ONE pixel of the screen.
// Thousands of them together make fire, sparks, smoke, rain...

// How a particle is written into the framebuffer.
enum class ParticleBlend
{
    Intensity, // Adds its colour to the pixel, fading out with age: glowing fire, sparks
    Opaque     // Replaces the pixel with its colour, like a tiny solid object: dust, debris
};

struct Particle
{
    math::Vector3<float> position;
    math::Vector3<float> velocity;              // units per second

    float age = 0.0f;                           // seconds since it was spawned
    float duration = 1.0f;                      // the particle dies when age reaches it

    math::Vector3<float> color{ 1.0f, 1.0f, 1.0f }; // (x, y, z) = (r, g, b) in [0, 1]
    ParticleBlend blend = ParticleBlend::Intensity;
};

// The forces shared by every particle of a system.
struct ParticlePhysics
{
    math::Vector3<float> gravity{ 0.0f, -1.0f, 0.0f }; // acceleration, units per second squared
    float drag = 0.0f;                                  // 0 = none, 1..5 = the particles slow down quickly
    float maxSpeed = 0.0f;                              // 0 = no limit

    // Optional box the particles bounce in (a floor, the walls of a room...)
    bool useBounds = false;
    math::Vector3<float> boundsMin;
    math::Vector3<float> boundsMax;
    float bounce = 0.0f;                                // speed kept after a bounce: 0 = stops dead, 1 = perfect bounce
};

// Every living particle, in a pool that never grows past maxCount.
// The reference stores each field in its own array (SoA) for SIMD; we keep one struct
// per particle (AoS) like the rest of the project, it is simpler to read.
struct ParticleSystem
{
    std::vector<Particle> particles;
    size_t maxCount = 0;
    ParticlePhysics physics;
};

// Spawns new particles around a point. The particles don't belong to it:
// once spawned they live on their own in the ParticleSystem, even if the emitter moves or stops.
struct ParticleEmitter
{
    ParticleBlend blend = ParticleBlend::Intensity;

    float rate = 100.0f;                               // particles per second, 0 = stopped
    float spawnRadius = 0.05f;                         // particles appear in a cube of this half size around position

    math::Vector3<float> position;                     // world space
    math::Vector3<float> direction{ 0.0f, 1.0f, 0.0f }; // unit length, where the particles are thrown
    math::Vector3<float> colorMin{ 1.0f, 1.0f, 1.0f }; // each particle picks a colour between these two
    math::Vector3<float> colorMax{ 1.0f, 1.0f, 1.0f };
    float durationMin = 0.5f;                          // lifetime range, in seconds
    float durationMax = 3.0f;
    float speedMin = 0.7f;                             // launch speed range
    float speedMax = 1.3f;

    // How much the launch direction is randomized:
    // 0        => laser, perfectly straight rain
    // 0.05-0.2 => channelled smoke, steam
    // 0.3-0.7  => fire, dust, sparks
    // > 1      => explosion, chaos
    float spread = 0.5f;

    float accumulator = 0.0f;                          // fraction of a particle left over from the previous frames
    uint32_t seed = 1;                                 // state of the random generator, give each emitter its own
};

namespace particle
{
    void create(ParticleSystem& system, size_t maxCount);

    // Spawns the particles this emitter owes for the last deltaTime seconds (as long as the pool has room).
    void emit(ParticleEmitter& emitter, ParticleSystem& system, float deltaTime);

    // Ages every particle, removes the dead ones, then moves the others (gravity, drag, bounds).
    void update(ParticleSystem& system, float deltaTime);

    // 1 when the particle is born, 0 when it dies: Intensity particles fade out with it.
    float lifeLeft(const Particle& particle);
}
