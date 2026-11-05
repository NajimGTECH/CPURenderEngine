#include "Light.h"

#include <algorithm>

math::Vector3<float> Light::incoming(const math::Vector3<float>& point, math::Vector3<float>& outToLight) const
{
    // The sun is so far away that every point sees it from the same direction, at full strength
    if (type == LightType::Directional)
    {
        outToLight = direction * -1.0f;
        return color * intensity;
    }

    // Point and spot lights spread out as they travel: the light gets weaker with the square of the distance.
    // The +1 keeps it from exploding when a surface is right next to the light.
    const math::Vector3<float> offset = position - point;
    outToLight = offset.normalized();
    float strength = intensity / (1.0f + offset.length_squared());

    if (type == LightType::Spot)
    {
        // Cosine of the angle between the spot axis and the ray going from the light to the point
        const float cosAngle = -outToLight.dot(direction);

        // 1 inside the inner cone, 0 outside the outer cone, linear in between
        const float cone = (cosAngle - outerConeCos) / (innerConeCos - outerConeCos);
        strength *= std::clamp(cone, 0.0f, 1.0f);
    }

    return color * strength;
}
