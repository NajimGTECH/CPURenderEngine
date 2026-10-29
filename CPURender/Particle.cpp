#include "Particle.h"

// Wang hash: scrambles the bits of a number, so consecutive seeds give unrelated results.
// Much cheaper than std::mt19937 when thousands of particles are spawned each second.
static uint32_t wangHash(uint32_t seed)
{
    seed = (seed ^ 61u) ^ (seed >> 16);
    seed *= 9u;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15);
    return seed;
}

// Random float in [0, 1). The seed moves forward, so the next call gives another number.
static float random01(uint32_t& seed)
{
    seed = wangHash(seed);
    return (seed & 0x00FFFFFFu) / 16777216.0f; // keep 24 bits: exactly what a float can hold
}

// Random float in [-1, 1)
static float randomSigned(uint32_t& seed)
{
    return random01(seed) * 2.0f - 1.0f;
}

// One random value per axis, each in [-1, 1)
static math::Vector3<float> randomSignedVector(uint32_t& seed)
{
    const float x = randomSigned(seed);
    const float y = randomSigned(seed);
    const float z = randomSigned(seed);
    return math::Vector3<float>(x, y, z);
}

// A particle that leaves the box is put back on its side, and its speed on that axis is reversed.
static void applyBounds(Particle& particle, const ParticlePhysics& physics)
{
    float* position[3] = { &particle.position.x, &particle.position.y, &particle.position.z };
    float* velocity[3] = { &particle.velocity.x, &particle.velocity.y, &particle.velocity.z };
    const float minimum[3] = { physics.boundsMin.x, physics.boundsMin.y, physics.boundsMin.z };
    const float maximum[3] = { physics.boundsMax.x, physics.boundsMax.y, physics.boundsMax.z };

    for (int axis = 0; axis < 3; axis++)
    {
        if (*position[axis] < minimum[axis])
        {
            *position[axis] = minimum[axis];
            *velocity[axis] = -*velocity[axis] * physics.bounce;
        }
        else if (*position[axis] > maximum[axis])
        {
            *position[axis] = maximum[axis];
            *velocity[axis] = -*velocity[axis] * physics.bounce;
        }
    }
}

void particle::create(ParticleSystem& system, size_t maxCount)
{
    // Reserve the whole pool once: spawning a particle never allocates memory during the game
    system.particles.clear();
    system.particles.reserve(maxCount);
    system.maxCount = maxCount;
}

void particle::emit(ParticleEmitter& emitter, ParticleSystem& system, float deltaTime)
{
    if (emitter.rate <= 0.0f) return;

    // At 100 particles per second and 60 FPS, a frame owes 1.67 particles:
    // spawn 1 now and keep the 0.67 for the next frame, so the rate stays exact.
    emitter.accumulator += emitter.rate * deltaTime;
    int count = static_cast<int>(emitter.accumulator);
    emitter.accumulator -= static_cast<float>(count);

    // A full pool simply drops the new particles
    while (count-- > 0 && system.particles.size() < system.maxCount)
    {
        Particle particle;

        // Somewhere in a small cube around the emitter
        particle.position = emitter.position + randomSignedVector(emitter.seed) * emitter.spawnRadius;

        // The emitter direction, bent at random by the spread, thrown at a random speed
        const float speed = emitter.speedMin + (emitter.speedMax - emitter.speedMin) * random01(emitter.seed);
        particle.velocity = (emitter.direction + randomSignedVector(emitter.seed) * emitter.spread) * speed;

        particle.duration = emitter.durationMin + (emitter.durationMax - emitter.durationMin) * random01(emitter.seed);

        // A colour between the two bounds, a bit darker or brighter so the cloud doesn't look flat
        const float colorRatio = random01(emitter.seed);
        const float intensity = 0.8f + 0.2f * random01(emitter.seed);
        particle.color = math::Vector3<float>::Lerp(emitter.colorMin, emitter.colorMax, colorRatio) * intensity;

        particle.blend = emitter.blend;
        system.particles.push_back(particle);
    }
}

void particle::update(ParticleSystem& system, float deltaTime)
{
    const ParticlePhysics& physics = system.physics;
    std::vector<Particle>& particles = system.particles;

    // Age: a dead particle is replaced by the last one of the list, then the list shrinks by one.
    // Nothing has to be shifted, but the order changes: that's fine, it doesn't mean anything here.
    size_t i = 0;
    while (i < particles.size())
    {
        particles[i].age += deltaTime;
        if (particles[i].age >= particles[i].duration)
        {
            particles[i] = particles.back();
            particles.pop_back();
            continue; // the particle moved into slot i hasn't been aged yet
        }
        i++;
    }

    // Drag removes a fraction of the speed every second.
    // 1 / (1 + drag * dt) instead of (1 - drag * dt): it can never go negative, even with a long frame.
    const float dragFactor = (physics.drag > 0.0f) ? 1.0f / (1.0f + physics.drag * deltaTime) : 1.0f;
    const float maxSpeedSquared = physics.maxSpeed * physics.maxSpeed;

    for (Particle& particle : particles)
    {
        math::Vector3<float> velocity = particle.velocity * dragFactor;
        velocity += physics.gravity * deltaTime;

        if (physics.maxSpeed > 0.0f && velocity.LengthSquared() > maxSpeedSquared)
        {
            velocity = velocity.Normalized() * physics.maxSpeed;
        }

        // Semi-implicit Euler: the position moves with the NEW velocity. Plain Euler (old velocity)
        // gains energy over time, and bouncing particles would jump higher and higher.
        particle.velocity = velocity;
        particle.position += velocity * deltaTime;

        if (physics.useBounds) applyBounds(particle, physics);
    }
}

float particle::lifeLeft(const Particle& particle)
{
    return 1.0f - particle.age / particle.duration;
}
