#include "Shader.h"

#include <algorithm>
#include <cmath>

// Light coming from everywhere at once (sky, bounces off the walls...), so the sides
// facing away from every light are not pitch black. Same level as chapter 10.
constexpr float AMBIENT_LIGHT = 0.18f;

// [0, 1] colour -> [0, 255] pixel colour. Several lights can add up past 1: clamp.
static Color3 toColor3(const math::Vector3<float>& color)
{
    return Color3{
        static_cast<uint8_t>(std::clamp(static_cast<int>(color.x * 255.0f), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(color.y * 255.0f), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(color.z * 255.0f), 0, 255))
    };
}

Color3 shader::shade(const Fragment& fragment, const Material& material,
    const std::vector<Light>& lights, const math::Vector3<float>& cameraPosition)
{
    // Base colour of the surface: the material colour, tinted by its texture if it has one
    math::Vector3<float> albedo = material.diffuse;
    if (material::hasTexture(material))
    {
        const Color3 texel = texture::sampleBilinear(material.diffuseMap, fragment.uv.u, fragment.uv.v);
        const math::Vector3<float> texelColor(texel.r / 255.0f, texel.g / 255.0f, texel.b / 255.0f);
        albedo = math::Vector3<float>::Scale(albedo, texelColor);
    }

    if (material.shading == ShadingModel::Unlit) return toColor3(albedo);

    // Ambient: like in chapter 10 it keeps the surface colour, so the shadowed side keeps its texture
    math::Vector3<float> color = math::Vector3<float>::Scale(material.ambient, albedo) * AMBIENT_LIGHT;

    // Direction from the surface towards the eye, for the highlight
    const math::Vector3<float> toCamera = (cameraPosition - fragment.position).Normalized();

    // Every light adds its own contribution
    for (const Light& light : lights)
    {
        math::Vector3<float> toLight;
        const math::Vector3<float> received = light::incoming(light, fragment.position, toLight);

        // Diffuse (Lambert): the more the surface faces the light, the more light it gets
        const float diffuse = std::max(0.0f, fragment.normal.Dot(toLight));
        color += math::Vector3<float>::Scale(albedo, received) * diffuse;

        // Specular (Blinn-Phong): a highlight where the normal points along the half vector H = normalize(L + V).
        // No highlight on a side the light doesn't reach.
        if (material.shading == ShadingModel::BlinnPhong && diffuse > 0.0f)
        {
            const math::Vector3<float> halfVector = (toLight + toCamera).Normalized();
            const float specular = std::pow(std::max(0.0f, fragment.normal.Dot(halfVector)), material.shininess);
            color += math::Vector3<float>::Scale(material.specular, received) * specular;
        }
    }

    return toColor3(color);
}
