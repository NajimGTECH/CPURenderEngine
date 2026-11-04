#include "Chapters.h"

#include "../Framebuffer.h"
#include "../Mat4.h"

#include <cmath>

// Chapter 4: first transformations, a triangle rotating inside a circle.
// Its small math classes are deprecated: everything after this chapter uses Vec3.h and Mat4.h.

constexpr int CIRCLE_RAY = 200;

// Math classes

#pragma region Chap4 maths (deprecated)
struct Vec3
{
    float x, y, z = 1.0f;

    float magnitude()
    {
        return std::sqrt(std::powf(x, 2.0f) + std::powf(y, 2.0f) + std::powf(z, 2.0f));
    }

    Vec3 normalized()
    {
        auto magnitude = this->magnitude();
        if (magnitude == 0.f) return *this;

        return { x / magnitude, y / magnitude, z / magnitude };
    }
};

Vec3 add_vectors(const Vec3& vecA, const Vec3& vecB)
{
    return Vec3{ vecA.x + vecB.x, vecA.y + vecB.y, vecA.z + vecB.z };
}

Vec3 sub_vectors(const Vec3& vecA, const Vec3& vecB)
{
    return Vec3{ vecA.x - vecB.x, vecA.y - vecB.y, vecA.z - vecB.z };
}

Vec3 scale_vectors(float s, const Vec3& vec)
{
    return Vec3{ s * vec.x, s * vec.y, s * vec.z };
}

float dot_product(const Vec3& vecA, const Vec3& vecB)
{
    return vecA.x * vecB.x + vecA.y * vecB.y + vecA.z * vecB.z;
}

Vec3 cross_product(const Vec3& vecA, const Vec3& vecB)
{
    return Vec3{ vecA.y * vecB.z - vecA.z * vecB.y,
        vecA.z * vecB.x - vecA.x * vecB.z,
        vecA.x * vecB.y - vecA.y * vecB.x };
}

struct Mat4
{
    float m[4][4]{};
};
#pragma endregion

void chapter_4_draw_circle(const Pixel& center, int ray)
{
    // Draw circle

    int x = 0;
    int y = ray;
    int diameter = 3 - 2 * ray;

    while (x <= y)
    {
        add_pixel_to_buffer(Pixel{ center.x + x, center.y + y, center.color });
        add_pixel_to_buffer(Pixel{ center.x - x, center.y + y, center.color });
        add_pixel_to_buffer(Pixel{ center.x + x, center.y - y, center.color });
        add_pixel_to_buffer(Pixel{ center.x - x, center.y - y, center.color });

        add_pixel_to_buffer(Pixel{ center.x + y, center.y + x, center.color });
        add_pixel_to_buffer(Pixel{ center.x - y, center.y + x, center.color });
        add_pixel_to_buffer(Pixel{ center.x + y, center.y - x, center.color });
        add_pixel_to_buffer(Pixel{ center.x - y, center.y - x, center.color });

        x++;

        if (diameter > 0)
        {
            y--;
            diameter += 4 * (x - y) + 10;
        }
        else
        {
            diameter += 4 * x + 6;
        }
    }
}

// Rotating RGB triangle inside its reference circle.
void chapter_4(float seconds)
{
    const float angle = seconds;

    // Define the triangle in local coordinates.
    const math::Vector3<float> baseA(
        0.0f, -static_cast<float>(CIRCLE_RAY), 0.0f);

    const float halfBase =
        static_cast<float>(CIRCLE_RAY) * 0.8660254f;

    const math::Vector3<float> baseB(
        halfBase, CIRCLE_RAY * 0.5f, 0.0f);

    const math::Vector3<float> baseC(
        -halfBase, CIRCLE_RAY * 0.5f, 0.0f);

    // Create the Z-axis rotation matrix.
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    const math::Mat4<float> rotation(
        c, s, 0.0f, 0.0f,
        -s, c, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    // Translate the triangle to the center of the screen.
    const math::Mat4<float> translation =
        math::Mat4<float>::translate(
            math::Vector3<float>(
                WIDTH * 0.5f,
                HEIGHT * 0.5f,
                0.0f
            )
        );

    // Apply rotation first, then translation.
    const math::Mat4<float> transform = rotation * translation;

    // Transform all three vertices.
    const auto transformedA = transform.multiply_point_affine(baseA);
    const auto transformedB = transform.multiply_point_affine(baseB);
    const auto transformedC = transform.multiply_point_affine(baseC);

    // Draw the reference circle.
    chapter_4_draw_circle(
        Pixel{
            static_cast<int>(WIDTH * 0.5f),
            static_cast<int>(HEIGHT * 0.5f),
            Color3{ 60, 60, 80, 255 }
        },
        CIRCLE_RAY
    );

    // Convert transformed vertices to pixels.
    Pixel pA{
        static_cast<int>(transformedA.x), static_cast<int>(transformedA.y),
        Color3{ 255, 0, 0, 255 }
    };

    Pixel pB{
        static_cast<int>(transformedB.x), static_cast<int>(transformedB.y),
        Color3{ 0, 255, 0, 255 }
    };

    Pixel pC{
        static_cast<int>(transformedC.x), static_cast<int>(transformedC.y),
        Color3{ 0, 0, 255, 255 }
    };

    // Rasterize the RGB triangle.
    chapter_3(pA, pB, pC);
}

