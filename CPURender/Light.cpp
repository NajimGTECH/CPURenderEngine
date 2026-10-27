#include "Light.h"

#include <algorithm>

math::Vector3<float> light::incoming(const Light& light, const math::Vector3<float>& position, math::Vector3<float>& outToLight)
{
    // The sun is so far away that every point sees it from the same direction, at full strength
    if (light.type == LightType::Directional)
    {
        outToLight = light.direction * -1.0f;
        return light.color * light.intensity;
    }

    // Point and spot lights spread out as they travel: the light gets weaker with the square of the distance.
    // The +1 keeps it from exploding when a surface is right next to the light.
    const math::Vector3<float> offset = light.position - position;
    outToLight = offset.Normalized();
    float strength = light.intensity / (1.0f + offset.LengthSquared());

    if (light.type == LightType::Spot)
    {
        // Cosine of the angle between the spot axis and the ray going from the light to the point
        const float cosAngle = -outToLight.Dot(light.direction);

        // 1 inside the inner cone, 0 outside the outer cone, linear in between
        const float cone = (cosAngle - light.outerConeCos) / (light.innerConeCos - light.outerConeCos);
        strength *= std::clamp(cone, 0.0f, 1.0f);
    }

    return light.color * strength;
}
