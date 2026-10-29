#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "Pixel.h"
#include "Vec3.h"
#include "Mat4.h"
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "Light.h"
#include "Shader.h"

#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <limits>
#include <numbers>
#include <thread>
#include <atomic>





// Screen

constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 600;
constexpr int CIRCLE_RAY = 200;


// Buffer

uint32_t frameBuffer[WIDTH * HEIGHT];

// Depth of the closest thing drawn so far on each pixel (chapter 9).
float depthBuffer[WIDTH * HEIGHT];

void add_pixel_to_buffer(Pixel px)
{
    if (px.x < 0 || px.x >= WIDTH || px.y < 0 || px.y >= HEIGHT) { SDL_Log("Pixel out of screen"); return; }

    frameBuffer[px.y * WIDTH + px.x] = px.color.toRGB();
}


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


// Chapters

void chapter_1()
{
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            Pixel pixel(x, y, Color3{ static_cast<uint8_t>(x * 255 / WIDTH), 64, static_cast<uint8_t>(y * 255 / HEIGHT) });
            add_pixel_to_buffer(pixel);
        }
    }
}

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

// The edge function: the 2D cross product of (B - A) and (P - A).
// Sign = which side of edge A->B the point P is on.
// Magnitude = twice the signed area of triangle (A, B, P).
// All integer math. 
float chapter_3_edge(const Pixel& a, const Pixel& b, const Pixel& p)
{
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

void chapter_3(Pixel& pA, Pixel& pB, Pixel& pC)
{
    float area = chapter_3_edge(pA, pB, pC);
    if (area == 0.f) return;
    if (area < 0.f)
    {
        std::swap(pB.x, pC.x);
        std::swap(pB.y, pC.y);
        std::swap(pB.color, pC.color);
        area = -area;
    }

    const float min_x = std::max(0, std::min({ pA.x, pB.x, pC.x }));
    const float min_y = std::max(0, std::min({ pA.y, pB.y, pC.y }));
    const float max_x = std::min(WIDTH - 1, std::max({ pA.x, pB.x, pC.x }));
    const float max_y = std::min(HEIGHT - 1, std::max({ pA.y, pB.y, pC.y }));

    for (int y = min_y; y < max_y; y++)
    {
        for (int x = min_x; x < max_x; x++)
        {
            Pixel point{ x, y };
            float edgeA = chapter_3_edge(pB, pC, point);
            float edgeB = chapter_3_edge(pC, pA, point);
            float edgeC = chapter_3_edge(pA, pB, point);

            // Inside if all three weights are non negative
            if (edgeA >= 0 and edgeB >= 0 and edgeC >= 0)
            {
                // Calculate normalized barycentric weights
                const float weightA = edgeA / area;
                const float weightB = edgeB / area;
                const float weightC = edgeC / area;

                // Interpolate the RGB channels
                const float red = weightA * pA.color.r + weightB * pB.color.r + weightC * pC.color.r;
                const float green = weightA * pA.color.g + weightB * pB.color.g + weightC * pC.color.g;
                const float blue = weightA * pA.color.b + weightB * pB.color.b + weightC * pC.color.b;

                // Clamp and round the channels to the valid RGB range
                point.color.r = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(red)), 0, 255));
                point.color.g = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(green)), 0, 255));
                point.color.b = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(blue)), 0, 255));

                add_pixel_to_buffer(point);
            }
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
        math::Mat4<float>::Translate(
            math::Vector3<float>(
                WIDTH * 0.5f,
                HEIGHT * 0.5f,
                0.0f
            )
        );

    // Apply rotation first, then translation.
    const math::Mat4<float> transform = rotation * translation;

    // Transform all three vertices.
    const auto transformedA = transform.MultiplyPointAffine(baseA);
    const auto transformedB = transform.MultiplyPointAffine(baseB);
    const auto transformedC = transform.MultiplyPointAffine(baseC);

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

// Bresenham line: integer-only walk from one pixel to the other.
// "error" tracks how far the ideal line is from the pixel we're on,
// and tells us when to step in x, in y, or in both.
void chapter_6_draw_line(const Pixel& from, const Pixel& to)
{
    int x = from.x;
    int y = from.y;

    const int dx = std::abs(to.x - from.x);
    const int dy = -std::abs(to.y - from.y);
    const int stepX = (from.x < to.x) ? 1 : -1;
    const int stepY = (from.y < to.y) ? 1 : -1;
    int error = dx + dy;

    while (true)
    {
        add_pixel_to_buffer(Pixel{ x, y, from.color });
        if (x == to.x && y == to.y) break;

        const int doubleError = 2 * error;
        if (doubleError >= dy) { error += dy; x += stepX; }
        if (doubleError <= dx) { error += dx; y += stepY; }
    }
}

// Spinning wireframe mesh loaded from an OBJ file.
// Every vertex goes through the 3D pipeline (model -> view -> projection -> divide -> viewport),
// then each triangle is drawn as 3 lines, shaded by depth (bright = near, dim = far).
void chapter_6(const Mesh& mesh, float seconds)
{
    // Projection: 50 degrees vertical field of view
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(fovY, aspect, 0.1f, 100.0f);

    // View: camera 3 units back, looking at the origin where the mesh sits, +Y is up
    const math::Mat4<float> view = math::Mat4<float>::LookAt(
        math::Vector3<float>(0.0f, 0.0f, 3.0f),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f)
    );

    // Model: spin around the Y axis
    const math::Mat4<float> model = math::Mat4<float>::Rotate(
        math::Quaternion::FromAxisAngle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f)
    );

    // Row vectors (p * M): model is applied first, then view.
    const math::Mat4<float> modelView = model * view;

    // Project every vertex once, faces will reuse them by index.
    struct ProjectedVertex
    {
        int x = 0, y = 0;
        float depth = 0.0f;   // distance in front of the camera (bigger = farther)
        bool visible = false; // false if behind the camera
    };

    std::vector<ProjectedVertex> projected(mesh.vertices.size());
    float minDepth = std::numeric_limits<float>::max();
    float maxDepth = std::numeric_limits<float>::lowest();

    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        // Model space -> camera space
        const math::Vector3<float> cameraPos = modelView.MultiplyPointAffine(mesh.vertices[i]);

        // The camera looks down -Z, so the distance in front of it is -z (this is also clip.w).
        // We shade by this depth instead of NDC z: NDC z barely changes across a small object,
        // every edge would end up the same grey.
        const float depth = -cameraPos.z;
        if (depth <= 0.0001f) continue;

        // Camera space -> NDC [-1, 1] (MultiplyPoint does the perspective divide by w)
        const math::Vector3<float> ndc = projection.MultiplyPoint(cameraPos);

        // NDC -> screen pixels (Y is flipped: up in NDC is down on screen)
        projected[i].x = static_cast<int>((ndc.x + 1.0f) * 0.5f * WIDTH);
        projected[i].y = static_cast<int>((1.0f - ndc.y) * 0.5f * HEIGHT);
        projected[i].depth = depth;
        projected[i].visible = true;

        minDepth = std::min(minDepth, depth);
        maxDepth = std::max(maxDepth, depth);
    }

    const float depthRange = (maxDepth > minDepth) ? (maxDepth - minDepth) : 1.0f;

    for (const Face& face : mesh.faces)
    {
        const ProjectedVertex& vA = projected[face.a];
        const ProjectedVertex& vB = projected[face.b];
        const ProjectedVertex& vC = projected[face.c];
        if (!vA.visible || !vB.visible || !vC.visible) continue;

        // Depth cue: map the mesh's own near..far span to a bright..dim grey,
        // so it stays readable whatever the distance to the camera.
        const float averageDepth = (vA.depth + vB.depth + vC.depth) / 3.0f;
        const float far01 = (averageDepth - minDepth) / depthRange; // 0 = near, 1 = far
        const uint8_t level = static_cast<uint8_t>(std::clamp(240 - static_cast<int>(far01 * 200.0f), 0, 255));
        const Color3 color{ level, level, level };

        const Pixel pA{ vA.x, vA.y, color };
        const Pixel pB{ vB.x, vB.y, color };
        const Pixel pC{ vC.x, vC.y, color };

        chapter_6_draw_line(pA, pB);
        chapter_6_draw_line(pB, pC);
        chapter_6_draw_line(pC, pA);
    }
}

// Chapter 8: Backface culling + Z-buffer


// These constants turn each technique on or off,
// to compare how much each one changes the render.
constexpr bool USE_ZBUFFER = true;
constexpr bool CULL_BACKFACES = true;

// Stores the depth of every pixel of the screen.
// With the projection matrix used here, a smaller depth
// means a closer surface.
float zBuffer[WIDTH * HEIGHT];

// Resets every depth before drawing a new frame.
// The maximum value lets the first visible surface
// pass the depth test.
void clear_zbuffer()
{
    std::fill_n(
        zBuffer,
        WIDTH * HEIGHT,
        std::numeric_limits<float>::max());
}

// Signed area between three 2D points A, B and P.
// The sign tells which side of edge AB the point P is on.
// Used to know whether a pixel is inside a triangle
// and to find the orientation of a triangle.
int chapter_7_edge(
    int ax, int ay,
    int bx, int by,
    int px, int py)
{
    return (bx - ax) * (py - ay)
        - (by - ay) * (px - ax);
}

// Clamps a value to a given range.
// Here it keeps the coordinates inside the framebuffer,
// so we never touch pixels outside the screen.
int chapter_7_clampi(int value, int minimum, int maximum)
{
    return std::clamp(value, minimum, maximum);
}

// Fills a triangle pixel by pixel.
// For each pixel of the triangle, computes its depth
// and optionally uses the Z-buffer to keep only the visible surface.
void chapter_7_draw_triangle(
    int x0, int y0, float z0,
    int x1, int y1, float z1,
    int x2, int y2, float z2,
    float zMin, float zRange)
{
    // Signed area of the triangle, from its three vertices.
    int area = chapter_7_edge(x0, y0, x1, y1, x2, y2);

    // A zero area means the triangle is flat in 2D:
    // there is no surface to fill.
    if (area == 0)
    {
        return;
    }

    // Flip the triangle's orientation so its area is positive.
    // The depths must follow their vertices,
    // so z1 and z2 are swapped too.
    if (area < 0)
    {
        std::swap(x1, x2);
        std::swap(y1, y2);
        std::swap(z1, z2);
        area = -area;
    }

    // Bounding box of the triangle:
    // the rectangle in which the triangle
    // can cover pixels.
    // The coordinates are clamped to the screen size.
    const int minX = chapter_7_clampi(
        std::min({ x0, x1, x2 }), 0, WIDTH - 1);

    const int minY = chapter_7_clampi(
        std::min({ y0, y1, y2 }), 0, HEIGHT - 1);

    const int maxX = chapter_7_clampi(
        std::max({ x0, x1, x2 }), 0, WIDTH - 1);

    const int maxY = chapter_7_clampi(
        std::max({ y0, y1, y2 }), 0, HEIGHT - 1);

    // Compute 1 / area once, so we don't redo
    // the same division for every pixel.
    const float inverseArea = 1.0f / static_cast<float>(area);

    // Walk every pixel of the bounding box.
    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            // Three edge values for the current pixel,
            // one per edge of the triangle.
            const int w0 = chapter_7_edge(
                x1, y1, x2, y2, x, y);

            const int w1 = chapter_7_edge(
                x2, y2, x0, y0, x, y);

            const int w2 = chapter_7_edge(
                x0, y0, x1, y1, x, y);

            // If any value is negative, the pixel is outside
            // the triangle: skip straight to the next pixel.
            if ((w0 | w1 | w2) < 0)
            {
                continue;
            }

            // Turn the edge values into barycentric weights.
            // They tell how much each vertex contributes
            // to the depth of the current pixel.
            const float weight0 =
                static_cast<float>(w0) * inverseArea;

            const float weight1 =
                static_cast<float>(w1) * inverseArea;

            const float weight2 =
                static_cast<float>(w2) * inverseArea;

            // Interpolate the pixel's depth from the depths
            // of the triangle's three vertices.
            const float depth =
                weight0 * z0
                + weight1 * z1
                + weight2 * z2;

            // Turn the pixel's 2D coordinates into an array index.
            // The framebuffer and the Z-buffer share the same index.
            const int index = y * WIDTH + x;

            // With the Z-buffer on, check whether the current pixel
            // is closer to the camera than the surface already stored.
            if constexpr (USE_ZBUFFER)
            {
                // If the depth is greater than or equal to the one
                // already stored, this surface is hidden or at the same
                // depth: don't draw this pixel.
                if (depth >= zBuffer[index])
                {
                    continue;
                }

                // The new surface is closer:
                // store its depth.
                zBuffer[index] = depth;
            }

            // Normalize the depth to [0, 1] to get
            // a greyscale effect:
            // near surfaces are bright, far surfaces are dark.
            const float normalizedDepth =
                std::clamp(
                    (depth - zMin) / zRange,
                    0.0f,
                    1.0f);

            const uint8_t intensity =
                static_cast<uint8_t>(
                    chapter_7_clampi(
                        240 - static_cast<int>(
                            normalizedDepth * 200.0f),
                        0,
                        255));

            // Write the pixel's final colour to the framebuffer.
            frameBuffer[index] =
                Color3{
                    intensity,
                    intensity,
                    intensity,
                    255
            }.toRGB();
        }
    }
}


// What we keep of a vertex once it is projected on the screen.
struct Chapter8ProjectedVertex
{
    int x = 0;          // Horizontal position on the screen.
    int y = 0;          // Vertical position on the screen.
    float depth = 0.0f; // Depth after projection.
    bool visible = false; // True if the vertex is in front of the camera.
};

// Solid render of a 3D model loaded from an OBJ file.
// Backface culling throws away the triangles facing away
// from the camera before they are rasterized.
//
// drawnTriangles counts the triangles that are kept.
// culledTriangles counts the triangles removed by backface culling.
void chapter_8(
    const Mesh& mesh,
    float seconds,
    int& drawnTriangles,
    int& culledTriangles)
{
    // Ratio between the screen's width and height.
    // It keeps the proportions right during the projection.
    const float aspect =
        static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);

    // Convert the 50 degree vertical field of view to radians.
    // The math functions work in radians.
    const float fovY =
        50.0f * std::numbers::pi_v<float> / 180.0f;

    // Perspective projection matrix.
    // 0.1 and 100.0 are the camera's near and far planes.
    const math::Mat4<float> projection =
        math::Mat4<float>::Perspective(
            fovY, aspect, 0.1f, 100.0f);

    // View matrix.
    // The camera sits at (0, 0, 3), looks at the origin
    // and uses the Y axis as up.
    const math::Mat4<float> view =
        math::Mat4<float>::LookAt(
            math::Vector3<float>(0.0f, 0.0f, 3.0f),
            math::Vector3<float>(0.0f, 0.0f, 0.0f),
            math::Vector3<float>(0.0f, 1.0f, 0.0f));

    // Rotation around the Y axis.
    // The angle grows with time, which animates the model.
    const math::Mat4<float> model =
        math::Mat4<float>::Rotate(
            math::Quaternion::FromAxisAngle(
                math::Vector3<float>(0.0f, 1.0f, 0.0f),
                seconds * 0.7f));

    // Combine the model and camera transforms.
    // With the vector convention used here, the model
    // is applied before the view.
    const math::Mat4<float> modelView = model * view;

    // Array of projected vertices,
    // one entry per vertex of the mesh.
    std::vector<Chapter8ProjectedVertex> projected(
        mesh.vertices.size());

    // Minimum and maximum depths,
    // used to compute the grey levels.
    float zMin = std::numeric_limits<float>::max();
    float zMax = std::numeric_limits<float>::lowest();

    // Walk every vertex of the 3D model.
    for (std::size_t i = 0; i < mesh.vertices.size(); ++i)
    {
        // Move the vertex into camera space.
        const math::Vector3<float> cameraPosition =
            modelView.MultiplyPointAffine(mesh.vertices[i]);

        // The camera looks down negative Z.
        // If -Z is too small, the vertex is behind the camera
        // or too close to the camera plane to be handled here.
        if (-cameraPosition.z <= 0.0001f)
        {
            continue;
        }

        // Apply the perspective projection and the divide
        // by the homogeneous coordinate that perspective needs.
        // The result holds the normalized (NDC) coordinates.
        const math::Vector3<float> ndc =
            projection.MultiplyPoint(cameraPosition);

        // Convert the NDC X coordinate, normally between -1 and 1,
        // into a horizontal screen coordinate.
        projected[i].x =
            static_cast<int>((ndc.x + 1.0f) * 0.5f * WIDTH);

        // Convert the NDC Y coordinate into a vertical one.
        // Y is flipped because screen coordinates grow downwards.
        projected[i].y =
            static_cast<int>((1.0f - ndc.y) * 0.5f * HEIGHT);

        // Keep the NDC depth for the Z-buffer.
        projected[i].depth = ndc.z;

        // Mark this vertex as processed and in front of the camera.
        projected[i].visible = true;

        // Update the extreme depths of the projected model.
        zMin = std::min(zMin, ndc.z);
        zMax = std::max(zMax, ndc.z);
    }

    // Span of the depths.
    // If every depth is the same, use 1 to avoid
    // a division by zero when normalizing.
    const float zRange =
        (zMax > zMin) ? (zMax - zMin) : 1.0f;

    // Reset the counters for this frame.
    drawnTriangles = 0;
    culledTriangles = 0;

    // Walk every triangle of the mesh.
    for (const Face& face : mesh.faces)
    {
        // Fetch the face's three vertices from their indices.
        const Chapter8ProjectedVertex& a = projected[face.a];
        const Chapter8ProjectedVertex& b = projected[face.b];
        const Chapter8ProjectedVertex& c = projected[face.c];

        // Skip the triangle if at least one of its vertices was not
        // projected because it was behind the camera.
        if (!a.visible || !b.visible || !c.visible)
        {
            continue;
        }

        // Signed area of the projected triangle.
        // Its sign gives the winding order of the vertices on screen.
        const int area = chapter_7_edge(
            a.x, a.y,
            b.x, b.y,
            c.x, c.y);

        // Skip degenerate triangles, or ones seen exactly edge-on.
        if (area == 0)
        {
            continue;
        }

        // With backface culling on, remove the triangles
        // whose winding means they are back faces.
        // The sign depends on the coordinate convention used.
        if constexpr (CULL_BACKFACES)
        {
            if (area >= 0)
            {
                // Count the culled triangle.
                ++culledTriangles;

                // Go straight to the next face.
                continue;
            }
        }

        // The triangle passed the orientation test,
        // so it is handed to the rasterizer.
        ++drawnTriangles;

        // Fill the triangle, using the Z-buffer so that
        // only the closest surface shows on each pixel.
        chapter_7_draw_triangle(
            a.x, a.y, a.depth,
            b.x, b.y, b.depth,
            c.x, c.y, c.depth,
            zMin, zRange);
    }
}


// Flip to false to interpolate the UVs the naive (affine) way:
// the texture warps and swims as the mesh turns, like on the PlayStation 1.
constexpr bool CHAPTER_9_PERSPECTIVE_CORRECT = true;

// A projected vertex with everything the textured fill needs.
struct TexturedVertex
{
    int x = 0, y = 0;  // screen position
    float z = 0.0f;    // NDC depth, for the depth test
    float invW = 0.0f; // 1 / w, for the perspective-correct UVs
    UV uv;             // texture coordinate of this corner
};

// Same barycentric fill as chapter 3, but instead of blending 3 colors we blend 3 texture
// coordinates, then read the color of the texture at that spot.
void chapter_9_draw_triangle_textured(TexturedVertex vA, TexturedVertex vB, TexturedVertex vC, const Texture& meshTexture)
{
    float area = chapter_3_edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y });
    if (area == 0.f) return;
    if (area < 0.f)
    {
        // Swap the whole vertex, so its depth and UV follow its position
        std::swap(vB, vC);
        area = -area;
    }

    const Pixel pA{ vA.x, vA.y };
    const Pixel pB{ vB.x, vB.y };
    const Pixel pC{ vC.x, vC.y };

    const int min_x = std::max(0, std::min({ pA.x, pB.x, pC.x }));
    const int min_y = std::max(0, std::min({ pA.y, pB.y, pC.y }));
    const int max_x = std::min(WIDTH - 1, std::max({ pA.x, pB.x, pC.x }));
    const int max_y = std::min(HEIGHT - 1, std::max({ pA.y, pB.y, pC.y }));

    // u and v do NOT vary linearly across the screen under perspective, but u/w, v/w and 1/w do.
    // So we pre-divide the UVs by w here, and divide back at each pixel.
    const float uOverWA = vA.uv.u * vA.invW, vOverWA = vA.uv.v * vA.invW;
    const float uOverWB = vB.uv.u * vB.invW, vOverWB = vB.uv.v * vB.invW;
    const float uOverWC = vC.uv.u * vC.invW, vOverWC = vC.uv.v * vC.invW;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            const Pixel point{ x, y };
            const float edgeA = chapter_3_edge(pB, pC, point);
            const float edgeB = chapter_3_edge(pC, pA, point);
            const float edgeC = chapter_3_edge(pA, pB, point);

            // Outside the triangle
            if (edgeA < 0 || edgeB < 0 || edgeC < 0) continue;

            const float weightA = edgeA / area;
            const float weightB = edgeB / area;
            const float weightC = edgeC / area;

            // Depth test: only draw if this pixel is closer than what's already there.
            // NDC z already varies linearly across the screen, so a plain blend is correct.
            const float z = weightA * vA.z + weightB * vB.z + weightC * vC.z;
            const int index = y * WIDTH + x;
            if (z >= depthBuffer[index]) continue;

            float u, v;
            if (CHAPTER_9_PERSPECTIVE_CORRECT)
            {
                // Blend u/w, v/w and 1/w, then divide back to get the true u, v
                const float invW = weightA * vA.invW + weightB * vB.invW + weightC * vC.invW;
                u = (weightA * uOverWA + weightB * uOverWB + weightC * uOverWC) / invW;
                v = (weightA * vOverWA + weightB * vOverWB + weightC * vOverWC) / invW;
            }
            else
            {
                // Naive blend, wrong under perspective
                u = weightA * vA.uv.u + weightB * vB.uv.u + weightC * vC.uv.u;
                v = weightA * vA.uv.v + weightB * vB.uv.v + weightC * vC.uv.v;
            }

            depthBuffer[index] = z;
            add_pixel_to_buffer(Pixel{ x, y, texture::sample(meshTexture, u, v) });
        }
    }
}


void chapter_10_draw_triangle_lit(TexturedVertex vA, TexturedVertex vB, TexturedVertex vC, const Texture& meshTexture, float brightness)
{
    float area = chapter_3_edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y });
    if (area == 0.f) return;
    if (area < 0.f)
    {
        // Swap the whole vertex, so its depth and UV follow its position
        std::swap(vB, vC);
        area = -area;
    }

    const Pixel pA{ vA.x, vA.y };
    const Pixel pB{ vB.x, vB.y };
    const Pixel pC{ vC.x, vC.y };

    const int min_x = std::max(0, std::min({ pA.x, pB.x, pC.x }));
    const int min_y = std::max(0, std::min({ pA.y, pB.y, pC.y }));
    const int max_x = std::min(WIDTH - 1, std::max({ pA.x, pB.x, pC.x }));
    const int max_y = std::min(HEIGHT - 1, std::max({ pA.y, pB.y, pC.y }));

    // u and v do NOT vary linearly across the screen under perspective, but u/w, v/w and 1/w do.
    // So we pre-divide the UVs by w here, and divide back at each pixel.
    const float uOverWA = vA.uv.u * vA.invW, vOverWA = vA.uv.v * vA.invW;
    const float uOverWB = vB.uv.u * vB.invW, vOverWB = vB.uv.v * vB.invW;
    const float uOverWC = vC.uv.u * vC.invW, vOverWC = vC.uv.v * vC.invW;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            const Pixel point{ x, y };
            const float edgeA = chapter_3_edge(pB, pC, point);
            const float edgeB = chapter_3_edge(pC, pA, point);
            const float edgeC = chapter_3_edge(pA, pB, point);

            // Outside the triangle
            if (edgeA < 0 || edgeB < 0 || edgeC < 0) continue;

            const float weightA = edgeA / area;
            const float weightB = edgeB / area;
            const float weightC = edgeC / area;

            // Depth test: only draw if this pixel is closer than what's already there.
            // NDC z already varies linearly across the screen, so a plain blend is correct.
            const float z = weightA * vA.z + weightB * vB.z + weightC * vC.z;
            const int index = y * WIDTH + x;
            if (z >= depthBuffer[index]) continue;

            float u, v;
            if (CHAPTER_9_PERSPECTIVE_CORRECT)
            {
                // Blend u/w, v/w and 1/w, then divide back to get the true u, v
                const float invW = weightA * vA.invW + weightB * vB.invW + weightC * vC.invW;
                u = (weightA * uOverWA + weightB * uOverWB + weightC * uOverWC) / invW;
                v = (weightA * vOverWA + weightB * vOverWB + weightC * vOverWC) / invW;
            }
            else
            {
                // Naive blend, wrong under perspective
                u = weightA * vA.uv.u + weightB * vB.uv.u + weightC * vC.uv.u;
                v = weightA * vA.uv.v + weightB * vB.uv.v + weightC * vC.uv.v;
            }

            depthBuffer[index] = z;

            // Sample the texture, then darken its three RGB channels.
            Color3 texel = texture::sample(meshTexture, u, v);
            texel.r = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.r * brightness), 0, 255));
            texel.g = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.g * brightness), 0, 255));
            texel.b = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.b * brightness), 0, 255));
            add_pixel_to_buffer(Pixel{ x, y, texel });
        }
    }
}



// Chapter 10: diffuse lighting (Lambert)

constexpr float CHAPTER_10_AMBIENT = 0.18f;

// Computes one unit normal per face, in the model's local space.
// The cross product of two edges gives a direction perpendicular to the triangle.
std::vector<math::Vector3<float>> chapter_10_compute_face_normals(const Mesh& mesh)
{
    std::vector<math::Vector3<float>> normals;
    normals.reserve(mesh.faces.size());

    for (const Face& face : mesh.faces)
    {
        const auto& a = mesh.vertices[face.a];
        const auto& b = mesh.vertices[face.b];
        const auto& c = mesh.vertices[face.c];

        const math::Vector3<float> e1(b.x - a.x, b.y - a.y, b.z - a.z);
        const math::Vector3<float> e2(c.x - a.x, c.y - a.y, c.z - a.z);

        math::Vector3<float> normal(
            e1.y * e2.z - e1.z * e2.y,
            e1.z * e2.x - e1.x * e2.z,
            e1.x * e2.y - e1.y * e2.x);

        const float length = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (length > 0.000001f)
        {
            normal.x /= length;
            normal.y /= length;
            normal.z /= length;
        }
        normals.push_back(normal);
    }
    return normals;
}

// Same pipeline as chapter 9, with a flat brightness computed for each triangle.
void chapter_10(const Mesh& mesh, const Texture& meshTexture,
    const std::vector<math::Vector3<float>>& faceNormals, float seconds)
{
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(fovY, aspect, 0.1f, 100.0f);

    const math::Mat4<float> view = math::Mat4<float>::LookAt(
        math::Vector3<float>(0.0f, 0.0f, 3.0f),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f));

    const math::Mat4<float> model = math::Mat4<float>::Rotate(
        math::Quaternion::FromAxisAngle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f));
    const math::Mat4<float> modelView = model * view;

    // Light direction: unit vector pointing from the surface towards the light.
    math::Vector3<float> light(0.4f, 0.8f, 0.45f);
    const float lightLength = std::sqrt(light.x * light.x + light.y * light.y + light.z * light.z);
    light.x /= lightLength;
    light.y /= lightLength;
    light.z /= lightLength;

    std::fill_n(depthBuffer, WIDTH * HEIGHT, std::numeric_limits<float>::max());

    std::vector<TexturedVertex> projected(mesh.vertices.size());
    std::vector<bool> visible(mesh.vertices.size(), false);

    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        const math::Vector3<float> cameraPos = modelView.MultiplyPointAffine(mesh.vertices[i]);
        const float w = -cameraPos.z;
        if (w <= 0.0001f) continue;

        const math::Vector3<float> ndc = projection.MultiplyPoint(cameraPos);
        projected[i].x = static_cast<int>((ndc.x + 1.0f) * 0.5f * WIDTH);
        projected[i].y = static_cast<int>((1.0f - ndc.y) * 0.5f * HEIGHT);
        projected[i].z = ndc.z;
        projected[i].invW = 1.0f / w;
        visible[i] = true;
    }

    for (size_t faceIndex = 0; faceIndex < mesh.faces.size(); ++faceIndex)
    {
        const Face& face = mesh.faces[faceIndex];
        if (!visible[face.a] || !visible[face.b] || !visible[face.c]) continue;

        TexturedVertex vA = projected[face.a];
        TexturedVertex vB = projected[face.b];
        TexturedVertex vC = projected[face.c];

        // The Y flip of the screen makes the area of front faces negative.
        // A positive area means a back face: don't draw it.
        if (chapter_3_edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y }) >= 0.0f) continue;

        vA.uv = (face.uvA >= 0) ? mesh.uvs[face.uvA] : UV{};
        vB.uv = (face.uvB >= 0) ? mesh.uvs[face.uvB] : UV{};
        vC.uv = (face.uvC >= 0) ? mesh.uvs[face.uvC] : UV{};

        // The model matrix is only a rotation here: it moves the normal
        // from local space to world space without changing its length.
        const math::Vector3<float> normal = model.MultiplyPointAffine(faceNormals[faceIndex]);
        float diffuse = normal.x * light.x + normal.y * light.y + normal.z * light.z;
        diffuse = std::max(0.0f, diffuse); // A face turned away from the light gets no diffuse light.
        const float brightness = CHAPTER_10_AMBIENT + (1.0f - CHAPTER_10_AMBIENT) * diffuse;

        chapter_10_draw_triangle_lit(vA, vB, vC, meshTexture, brightness);
    }
}

// Spinning textured mesh: filled triangles, a depth buffer so near triangles hide far ones,
// backface culling, and a texture wrapped on it with perspective-correct UVs.
void chapter_9(const Mesh& mesh, const Texture& meshTexture, float seconds)
{
    // Same camera and spin as chapter 6
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(fovY, aspect, 0.1f, 100.0f);

    const math::Mat4<float> view = math::Mat4<float>::LookAt(
        math::Vector3<float>(0.0f, 0.0f, 3.0f),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f)
    );

    const math::Mat4<float> model = math::Mat4<float>::Rotate(
        math::Quaternion::FromAxisAngle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f)
    );

    const math::Mat4<float> modelView = model * view;

    // Reset the depth buffer: nothing drawn yet = infinitely far
    std::fill_n(depthBuffer, WIDTH * HEIGHT, std::numeric_limits<float>::max());

    // Project every vertex once, faces will reuse them by index.
    std::vector<TexturedVertex> projected(mesh.vertices.size());
    std::vector<bool> visible(mesh.vertices.size(), false);

    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        // Model space -> camera space
        const math::Vector3<float> cameraPos = modelView.MultiplyPointAffine(mesh.vertices[i]);

        // The camera looks down -Z, so w (the distance in front of it) is -z
        const float w = -cameraPos.z;
        if (w <= 0.0001f) continue;

        // Camera space -> NDC [-1, 1] (MultiplyPoint does the perspective divide by w)
        const math::Vector3<float> ndc = projection.MultiplyPoint(cameraPos);

        // NDC -> screen pixels (Y is flipped: up in NDC is down on screen)
        projected[i].x = static_cast<int>((ndc.x + 1.0f) * 0.5f * WIDTH);
        projected[i].y = static_cast<int>((1.0f - ndc.y) * 0.5f * HEIGHT);
        projected[i].z = ndc.z;
        projected[i].invW = 1.0f / w;
        visible[i] = true;
    }

    for (const Face& face : mesh.faces)
    {
        if (!visible[face.a] || !visible[face.b] || !visible[face.c]) continue;

        TexturedVertex vA = projected[face.a];
        TexturedVertex vB = projected[face.b];
        TexturedVertex vC = projected[face.c];

        // Backface culling: OBJ front faces are counter-clockwise, and the Y flip of the viewport
        // makes them clockwise on screen, so their area is negative.
        // Positive area = we're looking at the back of the triangle, it's hidden anyway: skip it.
        if (chapter_3_edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y }) >= 0.f) continue;

        // The UV belongs to the face corner, not the vertex: the same vertex can have
        // different UVs on two faces (on a seam of the texture)
        vA.uv = (face.uvA >= 0) ? mesh.uvs[face.uvA] : UV{};
        vB.uv = (face.uvB >= 0) ? mesh.uvs[face.uvB] : UV{};
        vC.uv = (face.uvC >= 0) ? mesh.uvs[face.uvC] : UV{};

        chapter_9_draw_triangle_textured(vA, vB, vC, meshTexture);
    }
}


// Chapter 11: smooth shading (Phong) + specular highlights

constexpr float CHAPTER_11_SPEC_STRENGTH = 0.55f; // 0 = no highlight, 1 = as bright as the light

// A projected vertex, like TexturedVertex, that also carries its normal (in world space).
struct ShadedVertex
{
    int x = 0, y = 0;            // screen position
    float z = 0.0f;              // NDC depth, for the depth test
    float invW = 0.0f;           // 1 / w, for perspective-correct interpolation
    UV uv;                       // texture coordinate of this corner
    math::Vector3<float> normal; // vertex normal, already rotated into world space
};

// One normal per VERTEX: the sum of the normals of the faces touching it, normalized.
// Two neighbouring triangles then share the same normal on their common corners:
// the interpolated normal no longer jumps from one triangle to the next, and the facets disappear.
std::vector<math::Vector3<float>> chapter_11_compute_vertex_normals(const Mesh& mesh,
    const std::vector<math::Vector3<float>>& faceNormals)
{
    std::vector<math::Vector3<float>> normals(mesh.vertices.size());

    for (size_t faceIndex = 0; faceIndex < mesh.faces.size(); ++faceIndex)
    {
        const Face& face = mesh.faces[faceIndex];
        normals[face.a] += faceNormals[faceIndex];
        normals[face.b] += faceNormals[faceIndex];
        normals[face.c] += faceNormals[faceIndex];
    }

    for (math::Vector3<float>& normal : normals) normal.Normalize();
    return normals;
}

// x^32 in 5 multiplications (x^2, x^4, x^8, x^16, x^32), much cheaper than std::pow in the per-pixel loop.
// The shininess is therefore fixed at 32.
float chapter_11_pow32(float x)
{
    const float x2 = x * x;
    const float x4 = x2 * x2;
    const float x8 = x4 * x4;
    const float x16 = x8 * x8;
    return x16 * x16;
}

// Same fill as chapter 9, but the lighting is computed for every PIXEL (Phong):
// the normal is interpolated (perspective-correct, like the UVs), re-normalized,
// then we evaluate Lambert diffuse and Blinn-Phong specular.
// bilinear picks the texture sampling (chapter 12).
void chapter_11_draw_triangle_phong(ShadedVertex vA, ShadedVertex vB, ShadedVertex vC, const Texture& meshTexture,
    const math::Vector3<float>& light, const math::Vector3<float>& halfVector, bool bilinear)
{
    float area = chapter_3_edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y });
    if (area == 0.f) return;
    if (area < 0.f)
    {
        // Swap the whole vertex, so its depth, UV and normal follow its position
        std::swap(vB, vC);
        area = -area;
    }

    const Pixel pA{ vA.x, vA.y };
    const Pixel pB{ vB.x, vB.y };
    const Pixel pC{ vC.x, vC.y };

    const int min_x = std::max(0, std::min({ pA.x, pB.x, pC.x }));
    const int min_y = std::max(0, std::min({ pA.y, pB.y, pC.y }));
    const int max_x = std::min(WIDTH - 1, std::max({ pA.x, pB.x, pC.x }));
    const int max_y = std::min(HEIGHT - 1, std::max({ pA.y, pB.y, pC.y }));

    // Like in chapter 9: every attribute interpolated under perspective is pre-divided by w,
    // normals included.
    const float uOverWA = vA.uv.u * vA.invW, vOverWA = vA.uv.v * vA.invW;
    const float uOverWB = vB.uv.u * vB.invW, vOverWB = vB.uv.v * vB.invW;
    const float uOverWC = vC.uv.u * vC.invW, vOverWC = vC.uv.v * vC.invW;
    const math::Vector3<float> nOverWA = vA.normal * vA.invW;
    const math::Vector3<float> nOverWB = vB.normal * vB.invW;
    const math::Vector3<float> nOverWC = vC.normal * vC.invW;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            const Pixel point{ x, y };
            const float edgeA = chapter_3_edge(pB, pC, point);
            const float edgeB = chapter_3_edge(pC, pA, point);
            const float edgeC = chapter_3_edge(pA, pB, point);

            // Outside the triangle
            if (edgeA < 0 || edgeB < 0 || edgeC < 0) continue;

            const float weightA = edgeA / area;
            const float weightB = edgeB / area;
            const float weightC = edgeC / area;

            // Depth test
            const float z = weightA * vA.z + weightB * vB.z + weightC * vC.z;
            const int index = y * WIDTH + x;
            if (z >= depthBuffer[index]) continue;

            const float invW = weightA * vA.invW + weightB * vB.invW + weightC * vC.invW;
            const float u = (weightA * uOverWA + weightB * uOverWB + weightC * uOverWC) / invW;
            const float v = (weightA * vOverWA + weightB * vOverWB + weightC * vOverWC) / invW;

            // An average of unit vectors is not unit length (it "sags" towards the centre):
            // re-normalize it, because the lighting dot products expect a unit normal.
            math::Vector3<float> normal = (nOverWA * weightA + nOverWB * weightB + nOverWC * weightC) / invW;
            normal.Normalize();

            // Diffuse (chapter 10), but per pixel
            const float diffuse = std::max(0.0f, normal.Dot(light));
            const float brightness = CHAPTER_10_AMBIENT + (1.0f - CHAPTER_10_AMBIENT) * diffuse;

            // Blinn-Phong specular: strongest when the normal points along the half vector
            const float specular = chapter_11_pow32(std::max(0.0f, normal.Dot(halfVector))) * CHAPTER_11_SPEC_STRENGTH;

            depthBuffer[index] = z;

            // texel * diffuse, plus a white highlight on top
            Color3 texel = bilinear ? texture::sampleBilinear(meshTexture, u, v) : texture::sample(meshTexture, u, v);
            texel.r = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.r * brightness + 255.0f * specular), 0, 255));
            texel.g = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.g * brightness + 255.0f * specular), 0, 255));
            texel.b = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.b * brightness + 255.0f * specular), 0, 255));
            add_pixel_to_buffer(Pixel{ x, y, texel });
        }
    }
}

// Same pipeline as chapter 10, with one normal per vertex and per-pixel lighting.
void chapter_11(const Mesh& mesh, const Texture& meshTexture,
    const std::vector<math::Vector3<float>>& vertexNormals, float seconds)
{
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(fovY, aspect, 0.1f, 100.0f);

    const math::Mat4<float> view = math::Mat4<float>::LookAt(
        math::Vector3<float>(0.0f, 0.0f, 3.0f),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f));

    const math::Mat4<float> model = math::Mat4<float>::Rotate(
        math::Quaternion::FromAxisAngle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f));
    const math::Mat4<float> modelView = model * view;

    // Same light as in chapter 10.
    const math::Vector3<float> light = math::Vector3<float>(0.4f, 0.8f, 0.45f).Normalized();

    // The camera is at z = 3 looking at the origin: seen from the surface, the eye is roughly towards +Z.
    // We keep that direction constant, so the half vector H = normalize(L + V) is computed once per frame.
    const math::Vector3<float> viewDirection(0.0f, 0.0f, 1.0f);
    const math::Vector3<float> halfVector = (light + viewDirection).Normalized();

    std::fill_n(depthBuffer, WIDTH * HEIGHT, std::numeric_limits<float>::max());

    std::vector<ShadedVertex> projected(mesh.vertices.size());
    std::vector<bool> visible(mesh.vertices.size(), false);

    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        const math::Vector3<float> cameraPos = modelView.MultiplyPointAffine(mesh.vertices[i]);
        const float w = -cameraPos.z;
        if (w <= 0.0001f) continue;

        const math::Vector3<float> ndc = projection.MultiplyPoint(cameraPos);
        projected[i].x = static_cast<int>((ndc.x + 1.0f) * 0.5f * WIDTH);
        projected[i].y = static_cast<int>((1.0f - ndc.y) * 0.5f * HEIGHT);
        projected[i].z = ndc.z;
        projected[i].invW = 1.0f / w;

        // The normal is a direction: rotate it with the model matrix, without translation.
        projected[i].normal = model.MultiplyVector(vertexNormals[i]);
        visible[i] = true;
    }

    for (const Face& face : mesh.faces)
    {
        if (!visible[face.a] || !visible[face.b] || !visible[face.c]) continue;

        ShadedVertex vA = projected[face.a];
        ShadedVertex vB = projected[face.b];
        ShadedVertex vC = projected[face.c];

        // Backface culling (see chapter 9)
        if (chapter_3_edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y }) >= 0.0f) continue;

        vA.uv = (face.uvA >= 0) ? mesh.uvs[face.uvA] : UV{};
        vB.uv = (face.uvB >= 0) ? mesh.uvs[face.uvB] : UV{};
        vC.uv = (face.uvC >= 0) ? mesh.uvs[face.uvC] : UV{};

        chapter_11_draw_triangle_phong(vA, vB, vC, meshTexture, light, halfVector, false);
    }
}


// Chapter 12: clipping against the near plane + bilinear filtering


// Set to false to go back to the blocky "nearest neighbour" sampling of chapter 9.
constexpr bool CHAPTER_12_BILINEAR = true;

// Distance between the camera and Spot. Set it to 0.4f to go inside the cow:
// the surface stays solid right up to the cut, instead of losing whole triangles.
constexpr float CHAPTER_12_CAMERA_DISTANCE = 3.0f;

// A vertex in clip space (after the projection, BEFORE the divide by w),
// with every attribute the clipping must interpolate and the rasterizer needs afterwards.
struct ClipVertex
{
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f; // clip space position
    UV uv;                                        // texture coordinate
    math::Vector3<float> normal;                  // world space normal
    math::Vector3<float> worldPosition;           // world space position (for the shaders)
};

// p * M with p = (x, y, z, 1), keeping w: MultiplyPoint would already divide by w,
// which is exactly what we must avoid until the triangle is clipped.
ClipVertex chapter_12_to_clip_space(const math::Mat4<float>& matrix, const math::Vector3<float>& p)
{
    ClipVertex vertex;
    vertex.x = p.x * matrix(0, 0) + p.y * matrix(1, 0) + p.z * matrix(2, 0) + matrix(3, 0);
    vertex.y = p.x * matrix(0, 1) + p.y * matrix(1, 1) + p.z * matrix(2, 1) + matrix(3, 1);
    vertex.z = p.x * matrix(0, 2) + p.y * matrix(1, 2) + p.z * matrix(2, 2) + matrix(3, 2);
    vertex.w = p.x * matrix(0, 3) + p.y * matrix(1, 3) + p.z * matrix(2, 3) + matrix(3, 3);
    return vertex;
}

// Interpolates ALL the attributes: a * (1 - t) + b * t.
ClipVertex chapter_12_lerp(const ClipVertex& a, const ClipVertex& b, float t)
{
    ClipVertex result;
    result.x = a.x + t * (b.x - a.x);
    result.y = a.y + t * (b.y - a.y);
    result.z = a.z + t * (b.z - a.z);
    result.w = a.w + t * (b.w - a.w);
    result.uv.u = a.uv.u + t * (b.uv.u - a.uv.u);
    result.uv.v = a.uv.v + t * (b.uv.v - a.uv.v);
    result.normal = a.normal + (b.normal - a.normal) * t;
    result.worldPosition = a.worldPosition + (b.worldPosition - a.worldPosition) * t;
    return result;
}

// Signed distance to the near plane, in clip space units. Positive = in front of the near plane (kept).
// In front of near <=> z / w >= -1 <=> z + w >= 0: the test works without dividing by w.
float chapter_12_near_distance(const ClipVertex& vertex)
{
    return vertex.z + vertex.w;
}

// Sutherland-Hodgman clipping of a triangle against the near plane.
// We walk the edges A->B, B->C, C->A and for each one we emit:
//  - both ends in front             -> end B
//  - A in front, B behind (leaving) -> the crossing point
//  - A behind, B in front (entering)-> the crossing point, then B
//  - both behind                    -> nothing
// The resulting polygon has 0, 3 or 4 vertices; a quad is split into 2 triangles (0,1,2) and (0,2,3).
// Writes 0, 3 or 6 vertices to out and returns the number of triangles (0, 1 or 2).
int chapter_12_clip_triangle_near(const ClipVertex& vA, const ClipVertex& vB, const ClipVertex& vC, ClipVertex out[6])
{
    const ClipVertex triangle[3] = { vA, vB, vC };
    ClipVertex polygon[4]; // a triangle cut by ONE plane gives at most 4 vertices
    int count = 0;

    for (int edge = 0; edge < 3; edge++)
    {
        const ClipVertex& a = triangle[edge];
        const ClipVertex& b = triangle[(edge + 1) % 3];
        const float distanceA = chapter_12_near_distance(a);
        const float distanceB = chapter_12_near_distance(b);
        const bool aInside = distanceA >= 0.0f;
        const bool bInside = distanceB >= 0.0f;

        // The edge crosses the plane: the crossing is at the fraction t where the distance is zero
        if (aInside != bInside) polygon[count++] = chapter_12_lerp(a, b, distanceA / (distanceA - distanceB));
        if (bInside) polygon[count++] = b;
    }

    if (count < 3) return 0;

    out[0] = polygon[0]; out[1] = polygon[1]; out[2] = polygon[2];
    if (count == 3) return 1;

    out[3] = polygon[0]; out[4] = polygon[2]; out[5] = polygon[3];
    return 2;
}

// An already clipped triangle (entirely in front of the near plane, so w > 0):
// divide by w, viewport, backface culling, then the Phong rasterizer of chapter 11.
void chapter_12_project_and_draw(const ClipVertex triangle[3], const Texture& meshTexture,
    const math::Vector3<float>& light, const math::Vector3<float>& halfVector)
{
    ShadedVertex projected[3];
    for (int k = 0; k < 3; k++)
    {
        const float invW = 1.0f / triangle[k].w;
        projected[k].x = static_cast<int>((triangle[k].x * invW + 1.0f) * 0.5f * WIDTH);
        projected[k].y = static_cast<int>((1.0f - triangle[k].y * invW) * 0.5f * HEIGHT);
        projected[k].z = triangle[k].z * invW;
        projected[k].invW = invW;
        projected[k].uv = triangle[k].uv;
        projected[k].normal = triangle[k].normal;
    }

    // Backface culling (see chapter 9)
    if (chapter_3_edge(Pixel{ projected[0].x, projected[0].y }, Pixel{ projected[1].x, projected[1].y },
        Pixel{ projected[2].x, projected[2].y }) >= 0.0f) return;

    chapter_11_draw_triangle_phong(projected[0], projected[1], projected[2], meshTexture, light, halfVector, CHAPTER_12_BILINEAR);
}

// Same render as chapter 11, but the pipeline is reorganized around clipping:
//  1) per mesh vertex: only the move to clip space and the world normal (no divide);
//  2) per triangle: near clipping -> 0, 1 or 2 triangles, then divide + viewport + culling + raster.
// Clipping can create NEW vertices (the crossing points), which is why
// the divide by w can no longer be done once per mesh vertex.
void chapter_12(const Mesh& mesh, const Texture& meshTexture,
    const std::vector<math::Vector3<float>>& vertexNormals, float seconds)
{
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(fovY, aspect, 0.1f, 100.0f);

    const math::Mat4<float> view = math::Mat4<float>::LookAt(
        math::Vector3<float>(0.0f, 0.0f, CHAPTER_12_CAMERA_DISTANCE),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f));

    const math::Mat4<float> model = math::Mat4<float>::Rotate(
        math::Quaternion::FromAxisAngle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f));

    // Row vectors (p * M): model, then view, then projection.
    const math::Mat4<float> modelViewProjection = model * view * projection;

    const math::Vector3<float> light = math::Vector3<float>(0.4f, 0.8f, 0.45f).Normalized();
    const math::Vector3<float> viewDirection(0.0f, 0.0f, 1.0f);
    const math::Vector3<float> halfVector = (light + viewDirection).Normalized();

    std::fill_n(depthBuffer, WIDTH * HEIGHT, std::numeric_limits<float>::max());

    // 1) Per vertex: clip space + world normal. No more need for the "visible" array:
    // clipping properly handles whatever is behind the camera.
    std::vector<ClipVertex> clipVertices(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        clipVertices[i] = chapter_12_to_clip_space(modelViewProjection, mesh.vertices[i]);
        clipVertices[i].normal = model.MultiplyVector(vertexNormals[i]);
    }

    // 2) Per triangle
    for (const Face& face : mesh.faces)
    {
        ClipVertex vA = clipVertices[face.a];
        ClipVertex vB = clipVertices[face.b];
        ClipVertex vC = clipVertices[face.c];

        vA.uv = (face.uvA >= 0) ? mesh.uvs[face.uvA] : UV{};
        vB.uv = (face.uvB >= 0) ? mesh.uvs[face.uvB] : UV{};
        vC.uv = (face.uvC >= 0) ? mesh.uvs[face.uvC] : UV{};

        ClipVertex clipped[6];
        const int triangleCount = chapter_12_clip_triangle_near(vA, vB, vC, clipped);
        for (int t = 0; t < triangleCount; t++)
        {
            chapter_12_project_and_draw(&clipped[t * 3], meshTexture, light, halfVector);
        }
    }
}

// ============================================================
// Week 2: Shaders and Materials (inspired by 3dzavr)
// ============================================================
// Until now the lighting was hard-coded in each rasterizer.
// Now the rasterizer only interpolates the attributes (position, normal, UV) and calls
// shader::shade for each pixel. The Material (loaded from a .mtl) picks the lighting
// equation, and the lights (sun, bulb, spotlight) are plain data.

// Keys 1, 2 and 3: turn each light on / off to see what it brings.
bool gSunLightOn = true;
bool gPointLightOn = true;
bool gSpotLightOn = true;

// Key T: makes every cow see-through (alpha blending, no depth writes).
// A material can also be transparent on its own with "d" below 1 in the .mtl:
// T only caps every opacity at TRANSPARENT_OPACITY.
bool gTransparencyEnabled = false;
constexpr float TRANSPARENT_OPACITY = 0.42f;

// A projected vertex, like ShadedVertex, that also carries its world position for the shader.
struct ShaderVertex
{
    int x = 0, y = 0;                   // screen position
    float z = 0.0f;                     // NDC depth, for the depth test
    float invW = 0.0f;                  // 1 / w, for perspective-correct interpolation
    UV uv;                              // texture coordinate of this corner
    math::Vector3<float> normal;        // world space normal
    math::Vector3<float> worldPosition; // world space position (point lights, highlight)
};

// "Over" blending: lays a colour on top of what is already in the framebuffer.
// result = color * opacity + background * (1 - opacity)
Color3 shader_blend_over_framebuffer(const Color3& color, int index, float opacity)
{
    // frameBuffer uses ARGB8888 (0xAARRGGBB).
    const uint32_t background = frameBuffer[index];
    const float a = std::clamp(opacity, 0.0f, 1.0f);
    const uint8_t backgroundR = static_cast<uint8_t>((background >> 16) & 0xFF);
    const uint8_t backgroundG = static_cast<uint8_t>((background >> 8) & 0xFF);
    const uint8_t backgroundB = static_cast<uint8_t>(background & 0xFF);

    return Color3{
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(color.r * a + backgroundR * (1.0f - a))), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(color.g * a + backgroundG * (1.0f - a))), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(color.b * a + backgroundB * (1.0f - a))), 0, 255))
    };
}

// Top-left fill rule: a pixel lying exactly on an edge shared by two triangles must be drawn by only ONE of them.
// Opaque triangles don't mind (the second one fails the depth test), but a transparent pixel
// blended twice shows up as a bright line along every edge of the mesh.
// The rule keeps the pixels on the top and left edges of a triangle and drops the others.
// With the winding left after the swap (clockwise on screen, Y down), a "top" edge is horizontal
// and goes right, a "left" edge goes up.
bool shader_is_top_left_edge(const Pixel& from, const Pixel& to)
{
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;
    return dy < 0 || (dy == 0 && dx > 0);
}

// Same fill as chapter 11, but the rasterizer no longer knows anything about lighting:
// it builds one Fragment per pixel and the shader decides its colour.
// opacity < 1 blends the pixel over the framebuffer instead of replacing it.
void shader_draw_triangle(ShaderVertex vA, ShaderVertex vB, ShaderVertex vC, const Material& material,
    const std::vector<Light>& lights, const math::Vector3<float>& cameraPosition, float opacity,
    int tileMinX, int tileMinY, int tileMaxX, int tileMaxY)
{
    float area = chapter_3_edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y });
    if (area == 0.f) return;
    if (area < 0.f)
    {
        // Swap the whole vertex, so all its attributes follow its position
        std::swap(vB, vC);
        area = -area;
    }

    const Pixel pA{ vA.x, vA.y };
    const Pixel pB{ vB.x, vB.y };
    const Pixel pC{ vC.x, vC.y };

    const int min_x = std::max(tileMinX, std::max(0, std::min({ pA.x, pB.x, pC.x })));
    const int min_y = std::max(tileMinY, std::max(0, std::min({ pA.y, pB.y, pC.y })));
    const int max_x = std::min(tileMaxX - 1, std::min(WIDTH - 1, std::max({ pA.x, pB.x, pC.x })));
    const int max_y = std::min(tileMaxY - 1, std::min(HEIGHT - 1, std::max({ pA.y, pB.y, pC.y })));

    const bool transparent = opacity < 1.0f;

    // Each edge value below is named after the vertex facing that edge (edgeA is edge B->C)
    const bool topLeftA = shader_is_top_left_edge(pB, pC);
    const bool topLeftB = shader_is_top_left_edge(pC, pA);
    const bool topLeftC = shader_is_top_left_edge(pA, pB);

    // Like in chapter 9: every attribute interpolated under perspective is pre-divided by w,
    // the world position included.
    const float uOverWA = vA.uv.u * vA.invW, vOverWA = vA.uv.v * vA.invW;
    const float uOverWB = vB.uv.u * vB.invW, vOverWB = vB.uv.v * vB.invW;
    const float uOverWC = vC.uv.u * vC.invW, vOverWC = vC.uv.v * vC.invW;
    const math::Vector3<float> nOverWA = vA.normal * vA.invW;
    const math::Vector3<float> nOverWB = vB.normal * vB.invW;
    const math::Vector3<float> nOverWC = vC.normal * vC.invW;
    const math::Vector3<float> pOverWA = vA.worldPosition * vA.invW;
    const math::Vector3<float> pOverWB = vB.worldPosition * vB.invW;
    const math::Vector3<float> pOverWC = vC.worldPosition * vC.invW;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            const Pixel point{ x, y };
            const float edgeA = chapter_3_edge(pB, pC, point);
            const float edgeB = chapter_3_edge(pC, pA, point);
            const float edgeC = chapter_3_edge(pA, pB, point);

            // Outside the triangle, or on an edge that belongs to the neighbouring triangle
            if (edgeA < 0 || edgeB < 0 || edgeC < 0) continue;
            if ((edgeA == 0 && !topLeftA) || (edgeB == 0 && !topLeftB) || (edgeC == 0 && !topLeftC)) continue;

            const float weightA = edgeA / area;
            const float weightB = edgeB / area;
            const float weightC = edgeC / area;

            // Depth test
            const float z = weightA * vA.z + weightB * vB.z + weightC * vC.z;
            const int index = y * WIDTH + x;
            if (z >= depthBuffer[index]) continue;

            // Blend the pre-divided attributes, then divide by the blended 1/w to get the real ones back
            const float invW = weightA * vA.invW + weightB * vB.invW + weightC * vC.invW;

            Fragment fragment;
            fragment.uv.u = (weightA * uOverWA + weightB * uOverWB + weightC * uOverWC) / invW;
            fragment.uv.v = (weightA * vOverWA + weightB * vOverWB + weightC * vOverWC) / invW;
            fragment.position = (pOverWA * weightA + pOverWB * weightB + pOverWC * weightC) / invW;
            fragment.normal = (nOverWA * weightA + nOverWB * weightB + nOverWC * weightC) / invW;
            fragment.normal.Normalize(); // see chapter 11

            Color3 color = shader::shade(fragment, material, lights, cameraPosition);

            if (transparent)
            {
                // Transparent fragments only TEST the depth, they don't write it:
                // a see-through surface must not hide what gets drawn after it.
                color = shader_blend_over_framebuffer(color, index, opacity);
            }
            else
            {
                depthBuffer[index] = z;
            }
            add_pixel_to_buffer(Pixel{ x, y, color });
        }
    }
}

// An already clipped triangle (against the near plane, chapter 12):
// divide by w, viewport, backface culling, then the shader rasterizer.
void shader_project_and_draw(const ClipVertex triangle[3], const Material& material,
    const std::vector<Light>& lights, const math::Vector3<float>& cameraPosition, float opacity,
    int tileMinX, int tileMinY, int tileMaxX, int tileMaxY)
{
    ShaderVertex projected[3];
    for (int k = 0; k < 3; k++)
    {
        const float invW = 1.0f / triangle[k].w;
        projected[k].x = static_cast<int>((triangle[k].x * invW + 1.0f) * 0.5f * WIDTH);
        projected[k].y = static_cast<int>((1.0f - triangle[k].y * invW) * 0.5f * HEIGHT);
        projected[k].z = triangle[k].z * invW;
        projected[k].invW = invW;
        projected[k].uv = triangle[k].uv;
        projected[k].normal = triangle[k].normal;
        projected[k].worldPosition = triangle[k].worldPosition;
    }

    // Backface culling (see chapter 9)
    if (chapter_3_edge(Pixel{ projected[0].x, projected[0].y }, Pixel{ projected[1].x, projected[1].y },
        Pixel{ projected[2].x, projected[2].y }) >= 0.0f) return;

    shader_draw_triangle(projected[0], projected[1], projected[2], material, lights, cameraPosition, opacity,
        tileMinX, tileMinY, tileMaxX, tileMaxY);
}


void shader_draw_mesh(const Mesh& mesh, const std::vector<math::Vector3<float>>& vertexNormals,
    const math::Mat4<float>& model, const math::Mat4<float>& viewProjection, const Material& material,
    const std::vector<Light>& lights, const math::Vector3<float>& cameraPosition, float opacity,
    int tileMinX, int tileMinY, int tileMaxX, int tileMaxY)
{
    // Row vectors (p * M): model, then view, then projection.
    const math::Mat4<float> modelViewProjection = model * viewProjection;

    std::vector<ClipVertex> clipVertices(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        clipVertices[i] = chapter_12_to_clip_space(modelViewProjection, mesh.vertices[i]);
        clipVertices[i].normal = model.MultiplyVector(vertexNormals[i]);
        clipVertices[i].worldPosition = model.MultiplyPointAffine(mesh.vertices[i]);
    }

    std::vector<size_t> faceOrder(mesh.faces.size());
    for (size_t i = 0; i < faceOrder.size(); ++i) faceOrder[i] = i;
    if (opacity < 1.0f)
    {
        auto averageW = [&](size_t i) {
            const Face& f = mesh.faces[i];
            return (clipVertices[f.a].w + clipVertices[f.b].w + clipVertices[f.c].w) / 3.0f;
            };
        std::stable_sort(faceOrder.begin(), faceOrder.end(), [&](size_t a, size_t b) {
            return averageW(a) > averageW(b); // farther first
            });
    }

    for (const size_t faceIndex : faceOrder)
    {
        const Face& face = mesh.faces[faceIndex];
        ClipVertex vA = clipVertices[face.a];
        ClipVertex vB = clipVertices[face.b];
        ClipVertex vC = clipVertices[face.c];

        vA.uv = (face.uvA >= 0) ? mesh.uvs[face.uvA] : UV{};
        vB.uv = (face.uvB >= 0) ? mesh.uvs[face.uvB] : UV{};
        vC.uv = (face.uvC >= 0) ? mesh.uvs[face.uvC] : UV{};

        ClipVertex clipped[6];
        const int triangleCount = chapter_12_clip_triangle_near(vA, vB, vC, clipped);
        for (int t = 0; t < triangleCount; t++)
        {
            shader_project_and_draw(&clipped[t * 3], material, lights, cameraPosition, opacity,
                tileMinX, tileMinY, tileMaxX, tileMaxY);
        }
    }
}

// The lights of the scene, rebuilt every frame because two of them move.
std::vector<Light> shader_make_lights(float seconds)
{
    std::vector<Light> lights;

    if (gSunLightOn)
    {
        // White sun, same direction as the light of chapters 10 to 12
        Light sun;
        sun.type = LightType::Directional;
        sun.direction = math::Vector3<float>(-0.4f, -0.8f, -0.45f).Normalized();
        sun.intensity = 0.8f;
        lights.push_back(sun);
    }

    if (gPointLightOn)
    {
        // Orange bulb circling around the scene
        Light bulb;
        bulb.type = LightType::Point;
        bulb.position = math::Vector3<float>(3.0f * std::cos(seconds), 0.5f, 3.0f * std::sin(seconds));
        bulb.color = math::Vector3<float>(1.0f, 0.55f, 0.2f);
        bulb.intensity = 5.0f;
        lights.push_back(bulb);
    }

    if (gSpotLightOn)
    {
        // Bluish spotlight above the camera, sweeping the scene from left to right
        Light spot;
        spot.type = LightType::Spot;
        spot.position = math::Vector3<float>(0.0f, 2.5f, 4.0f);
        const math::Vector3<float> target(2.0f * std::sin(seconds * 0.8f), -0.1f, 0.0f);
        spot.direction = (target - spot.position).Normalized();
        spot.color = math::Vector3<float>(0.5f, 0.75f, 1.0f);
        spot.intensity = 20.0f;
        spot.innerConeCos = std::cos(8.0f * std::numbers::pi_v<float> / 180.0f);
        spot.outerConeCos = std::cos(14.0f * std::numbers::pi_v<float> / 180.0f);
        lights.push_back(spot);
    }

    return lights;
}

// The same cow drawn once per material of the .mtl, on a 2x2 grid:
// top row unlit then Lambert, bottom row textured Blinn-Phong then untextured gold.
// Press T and the four of them turn see-through (transparent pass).
void shader_demo(const Mesh& mesh, const std::vector<math::Vector3<float>>& vertexNormals,
    const std::vector<Material>& materials, float seconds)
{
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(fovY, aspect, 0.1f, 100.0f);

    // Camera pulled back to see the 4 cows. The shader needs its position for the highlights.
    const math::Vector3<float> cameraPosition(0.0f, 0.0f, 4.2f);
    const math::Mat4<float> view = math::Mat4<float>::LookAt(
        cameraPosition,
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f));
    const math::Mat4<float> viewProjection = view * projection;

    const std::vector<Light> lights = shader_make_lights(seconds);

    // Every cow shares the same depth buffer: clear it only once.
    std::fill_n(depthBuffer, WIDTH * HEIGHT, std::numeric_limits<float>::max());

    // One grid cell per material, the cows all spin together.
    const math::Vector3<float> slots[] = {
        { -1.1f, 0.85f, 0.0f }, { 1.1f, 0.85f, 0.0f },
        { -1.1f, -1.05f, 0.0f }, { 1.1f, -1.05f, 0.0f } };
    const math::Quaternion spin = math::Quaternion::FromAxisAngle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f);
    const math::Vector3<float> scale(0.7f, 0.7f, 0.7f);

    struct Cow
    {
        math::Mat4<float> model;
        const Material* material = nullptr;
        float opacity = 1.0f;
        float distanceToCamera = 0.0f; // squared, only used to sort
    };

    std::vector<Cow> cows;
    for (size_t i = 0; i < materials.size() && i < std::size(slots); i++)
    {
        Cow cow;
        cow.model = math::Mat4<float>::TRS(slots[i], spin, scale);
        cow.material = &materials[i];
        cow.opacity = gTransparencyEnabled ? std::min(materials[i].opacity, TRANSPARENT_OPACITY) : materials[i].opacity;
        cow.distanceToCamera = (slots[i] - cameraPosition).LengthSquared();
        cows.push_back(cow);
    }

    // ============================================================
    // Window partitioning with multithreading.
    // Each worker owns one tile, so framebuffer and depth-buffer writes
    // never overlap between threads. Each tile completes the opaque pass
    // before its transparent pass, preserving per-pixel compositing order.
    // ============================================================
    constexpr int TILE_SIZE = 100;
    const int tilesX = (WIDTH + TILE_SIZE - 1) / TILE_SIZE;
    const int tilesY = (HEIGHT + TILE_SIZE - 1) / TILE_SIZE;
    const int totalTiles = tilesX * tilesY;

    std::vector<Cow> opaqueCows;
    std::vector<Cow> transparentCows;
    opaqueCows.reserve(cows.size());
    transparentCows.reserve(cows.size());

    for (const Cow& cow : cows)
    {
        if (cow.opacity >= 1.0f) opaqueCows.push_back(cow);
        else transparentCows.push_back(cow);
    }

    std::stable_sort(transparentCows.begin(), transparentCows.end(),
        [](const Cow& a, const Cow& b) {
            return a.distanceToCamera > b.distanceToCamera;
        });

    std::atomic<int> nextTile{ 0 };
    unsigned int workerCount = std::thread::hardware_concurrency();
    if (workerCount == 0) workerCount = 4;
    workerCount = std::min(workerCount, static_cast<unsigned int>(totalTiles));

    auto renderWorker = [&]() {
        while (true)
        {
            const int tileId = nextTile.fetch_add(1, std::memory_order_relaxed);
            if (tileId >= totalTiles) break;

            const int tileX = tileId % tilesX;
            const int tileY = tileId / tilesX;
            const int tileMinX = tileX * TILE_SIZE;
            const int tileMinY = tileY * TILE_SIZE;
            const int tileMaxX = std::min(tileMinX + TILE_SIZE, static_cast<int>(WIDTH));
            const int tileMaxY = std::min(tileMinY + TILE_SIZE, static_cast<int>(HEIGHT));

            // PASS 1: opaque objects write depth and colour.
            for (const Cow& cow : opaqueCows)
            {
                shader_draw_mesh(mesh, vertexNormals, cow.model, viewProjection,
                    *cow.material, lights, cameraPosition, cow.opacity,
                    tileMinX, tileMinY, tileMaxX, tileMaxY);
            }

            // PASS 2: transparent objects, back-to-front, test depth but don't write it.
            for (const Cow& cow : transparentCows)
            {
                shader_draw_mesh(mesh, vertexNormals, cow.model, viewProjection,
                    *cow.material, lights, cameraPosition, cow.opacity,
                    tileMinX, tileMinY, tileMaxX, tileMaxY);
            }
        }
        };

    std::vector<std::thread> workers;
    workers.reserve(workerCount);
    for (unsigned int i = 0; i < workerCount; ++i)
        workers.emplace_back(renderWorker);
    for (std::thread& worker : workers)
        worker.join();
}

// =============
// Renderer custom functions

// Clears the frame, draws the current chapter, then sends the framebuffer to the screen.
void update_render(SDL_Renderer* renderer, SDL_Texture* texture, float seconds, const Mesh& mesh, const Texture& meshTexture,
    const std::vector<math::Vector3<float>>& faceNormals, const std::vector<math::Vector3<float>>& vertexNormals,
    const std::vector<Material>& materials)
{
    // Clear the CPU framebuffer.
    std::fill_n(frameBuffer, WIDTH * HEIGHT, Color3{ 16, 16, 24, 255 }.toRGB());

    // Draw the current chapter. The week 2 shader demo is the default merged result.
    // To go back to an older chapter, uncomment its call and comment out shader_demo.
    int drawnTriangles = 0;
    int culledTriangles = 0;
    //chapter_8(mesh, seconds, drawnTriangles, culledTriangles);
    //chapter_9(mesh, meshTexture, seconds);
    //chapter_10(mesh, meshTexture, faceNormals, seconds);
    //chapter_11(mesh, meshTexture, vertexNormals, seconds);
    //chapter_12(mesh, meshTexture, vertexNormals, seconds);

    // Week 2: shaders and materials (keys 1, 2 and 3 for the lights, T for transparency)
    shader_demo(mesh, vertexNormals, materials, seconds);

    // Upload the CPU framebuffer, then draw readable UI text on top using SDL3's
    // built-in debug font (no additional TTF dependency required).
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(texture, nullptr, frameBuffer, WIDTH * sizeof(uint32_t));
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    char line[128];
    SDL_RenderDebugText(renderer, 12.0f, 12.0f, "3Dzavr CPU Renderer | Shaders + Materials");
    if (gTransparencyEnabled)
        SDL_snprintf(line, sizeof(line), "Pass: TRANSPARENT | alpha = %.2f | press T to switch", TRANSPARENT_OPACITY);
    else
        SDL_snprintf(line, sizeof(line), "Pass: OPAQUE | press T to switch to transparency");
    SDL_RenderDebugText(renderer, 12.0f, 26.0f, line);
    SDL_snprintf(line, sizeof(line), "Lights: 1 sun %s | 2 bulb %s | 3 spot %s",
        gSunLightOn ? "ON" : "OFF", gPointLightOn ? "ON" : "OFF", gSpotLightOn ? "ON" : "OFF");
    SDL_RenderDebugText(renderer, 12.0f, 40.0f, line);
    SDL_RenderDebugText(renderer, 12.0f, 54.0f, "ESC: quit");
    SDL_RenderPresent(renderer);
}


// Main function

int main()
{
    // Load the mesh and its texture once, before the loop (chapters 6 and 9)
    Mesh spot;
    if (!mesh::loadObj("assets/spot.obj", spot)) return EXIT_FAILURE;
    std::cout << "Loaded spot.obj:\t" << spot.vertices.size() << " vertices, " << spot.uvs.size() << " uvs, " << spot.faces.size() << " faces\n";

    Texture spotTexture;
    if (!texture::loadPng("assets/spot_texture.png", spotTexture)) return EXIT_FAILURE;
    std::cout << "Loaded spot_texture.png:\t" << spotTexture.width << "x" << spotTexture.height << " texels\n";

    // Week 2: one material per cow in the shader demo
    std::vector<Material> spotMaterials;
    if (!material::loadMtl("assets/spot.mtl", spotMaterials)) return EXIT_FAILURE;
    std::cout << "Loaded spot.mtl:\t" << spotMaterials.size() << " materials\n";

    // The normals don't depend on the rotation: compute them only once.
    const std::vector<math::Vector3<float>> faceNormals = chapter_10_compute_face_normals(spot);

    // Chapters 11 and 12: one normal per vertex, the average of the neighbouring face normals.
    const std::vector<math::Vector3<float>> vertexNormals = chapter_11_compute_vertex_normals(spot, faceNormals);

    // Init
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        SDL_Log("Couldn't init SDL\n");
        return EXIT_FAILURE;
    }

    SDL_Window* window = SDL_CreateWindow("Test Window", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
    if (window == nullptr) { SDL_Log("Window could not be created.\n"); return EXIT_FAILURE; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) { SDL_Log("Renderer could not be created.\n"); return EXIT_FAILURE; }
    std::cout << "Initialized renderer:\t" << SDL_GetRendererName(renderer) << '\n';

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (texture == nullptr) { SDL_Log("Texture could not be created.\n"); return EXIT_FAILURE; }

    SDL_Event event;

    // FPS measurement
    const Uint64 freq = SDL_GetPerformanceFrequency();
    const Uint64 start = SDL_GetPerformanceCounter();
    Uint64 last = start;
    double fps_accum = 0.0;
    int    fps_count = 0;
    char   title[160];

    // Loop
    bool isRunning = true;
    while (isRunning)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                isRunning = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.key == SDLK_ESCAPE)
                {
                    isRunning = false;
                }
                else if (event.key.key == SDLK_T)
                {
                    gTransparencyEnabled = !gTransparencyEnabled;
                }
                else if (event.key.key == SDLK_1)
                {
                    gSunLightOn = !gSunLightOn;
                }
                else if (event.key.key == SDLK_2)
                {
                    gPointLightOn = !gPointLightOn;
                }
                else if (event.key.key == SDLK_3)
                {
                    gSpotLightOn = !gSpotLightOn;
                }
            }
        }

        // Elapsed time, used by the chapters to animate.
        const float seconds = static_cast<float>(
            static_cast<double>(SDL_GetPerformanceCounter() - start)
            / static_cast<double>(freq));

        // Draw the frame (the current chapter is picked inside update_render).
        update_render(renderer, texture, seconds, spot, spotTexture, faceNormals, vertexNormals, spotMaterials);

        // FPS in the window title, refreshed once per second.
        const Uint64 now = SDL_GetPerformanceCounter();
        fps_accum += static_cast<double>(now - last) / static_cast<double>(freq);
        fps_count++;
        last = now;

        if (fps_accum >= 1.0)
        {
            const double fps = fps_count / fps_accum;
            const double ms = 1000.0 * fps_accum / fps_count;
            SDL_snprintf(title, sizeof(title), "Shaders - sun %s, bulb %s, spot %s (keys 1/2/3) | %s (key T) | %.1f FPS (%.2f ms)",
                gSunLightOn ? "ON" : "OFF", gPointLightOn ? "ON" : "OFF", gSpotLightOn ? "ON" : "OFF",
                gTransparencyEnabled ? "transparent" : "opaque", fps, ms);
            SDL_SetWindowTitle(window, title);
            fps_accum = 0.0;
            fps_count = 0;
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}