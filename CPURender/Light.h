#pragma once

#include "Vec3.h"

// The three kinds of light of the reference engine (3dzavr):
//  - Directional: infinitely far away (the sun), same direction and strength everywhere
//  - Point: a bulb at a position, shining in every direction, weaker with distance
//  - Spot: a point light that only shines inside a cone
enum class LightType
{
    Directional,
    Point,
    Spot
};

// One struct for every light type: each type only reads the fields it needs.
struct Light
{
    LightType type = LightType::Directional;

    math::Vector3<float> position;                        // Point, Spot
    math::Vector3<float> direction{ 0.0f, -1.0f, 0.0f };  // Directional, Spot: where the light travels, unit length

    math::Vector3<float> color{ 1.0f, 1.0f, 1.0f }; // (x, y, z) = (r, g, b) in [0, 1]
    float intensity = 1.0f;

    // Spot cone, stored as cosines of the angle from the spot axis (cos(10 deg) = 0.985).
    // Full light inside the inner cone, no light outside the outer one, a smooth ramp in between.
    float innerConeCos = 0.95f;
    float outerConeCos = 0.90f;
};

namespace light
{
    // Light arriving at a surface point.
    // outToLight receives the unit direction from the point towards the light.
    // Returns the light colour scaled by how much of it reaches the point (distance falloff, spot cone).
    math::Vector3<float> incoming(const Light& light, const math::Vector3<float>& position, math::Vector3<float>& outToLight);
}
