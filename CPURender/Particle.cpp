#include "Particle.h"

// Wang hash: scrambles the bits of a number, so consecutive seeds give unrelated results.
// Much cheaper than std::mt19937 when thousands of particles are spawned each second.
static uint32_t wang_hash(uint32_t seed)
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
    seed = wang_hash(seed);
    return (seed & 0x00FFFFFFu) / 16777216.0f; // keep 24 bits: exactly what a float can hold
}

// Random float in [-1, 1)
static float random_signed(uint32_t& seed)
{
    return random01(seed) * 2.0f - 1.0f;
}

// One random value per axis, each in [-1, 1)
static math::Vector3<float> random_signed_vector(uint32_t& seed)
{
    const float x = random_signed(seed);
    const float y = random_signed(seed);
    const float z = random_signed(seed);
    return math::Vector3<float>(x, y, z);
}

// A particle that leaves the box is put back on its side, and its speed on that axis is reversed.
static void apply_bounds(Particle& particle, const ParticlePhysics& physics)
{
    using Vector = math::Vector3<float>;

    for (float Vector::* axis : { &Vector::x, &Vector::y, &Vector::z })
    {
        float& position = particle.position.*axis;
        float& velocity = particle.velocity.*axis;
        const float minimum = physics.boundsMin.*axis;
        const float maximum = physics.boundsMax.*axis;

        if (position < minimum)
        {
            position = minimum;
            velocity = -velocity * physics.bounce;
        }
        else if (position > maximum)
        {
            position = maximum;
            velocity = -velocity * physics.bounce;
        }
    }
}

void ParticleSystem::reserve(size_t capacity)
{
    // Reserve the whole pool once: spawning a particle never allocates memory during the game
    particles.clear();
    particles.reserve(capacity);
    maxCount = capacity;
}

void ParticleEmitter::emit(ParticleSystem& system, float deltaTime)
{
    if (rate <= 0.0f) return;

    // At 100 particles per second and 60 FPS, a frame owes 1.67 particles:
    // spawn 1 now and keep the 0.67 for the next frame, so the rate stays exact.
    accumulator += rate * deltaTime;
    int count = static_cast<int>(accumulator);
    accumulator -= static_cast<float>(count);

    // A full pool simply drops the new particles
    while (count-- > 0 && system.particles.size() < system.maxCount)
    {
        Particle particle;

        // Somewhere in a small cube around the emitter
        particle.position = position + random_signed_vector(seed) * spawnRadius;

        // The emitter direction, bent at random by the spread, thrown at a random speed
        const float speed = speedMin + (speedMax - speedMin) * random01(seed);
        particle.velocity = (direction + random_signed_vector(seed) * spread) * speed;

        particle.duration = durationMin + (durationMax - durationMin) * random01(seed);

        // A colour between the two bounds, a bit darker or brighter so the cloud doesn't look flat
        const float colorRatio = random01(seed);
        const float intensity = 0.8f + 0.2f * random01(seed);
        particle.color = math::Vector3<float>::lerp(colorMin, colorMax, colorRatio) * intensity;

        particle.blend = blend;
        system.particles.push_back(particle);
    }
}

void ParticleSystem::update(float deltaTime)
{
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

        if (physics.maxSpeed > 0.0f && velocity.length_squared() > maxSpeedSquared)
        {
            velocity = velocity.normalized() * physics.maxSpeed;
        }

        // Semi-implicit Euler: the position moves with the NEW velocity. Plain Euler (old velocity)
        // gains energy over time, and bouncing particles would jump higher and higher.
        particle.velocity = velocity;
        particle.position += velocity * deltaTime;

        if (physics.useBounds) apply_bounds(particle, physics);
    }
}

float Particle::life_left() const
{
    return 1.0f - age / duration;
}
