#include "Chapters.h"

#include "../Framebuffer.h"
#include "../Mat4.h"
#include "../Rasterizer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>

// Chapter 8: Backface culling + Z-buffer


// These constants turn each technique on or off,
// to compare how much each one changes the render.
constexpr bool USE_ZBUFFER = true;
constexpr bool CULL_BACKFACES = true;

// Stores the depth of every pixel of the screen.
// With the projection matrix used here, a smaller depth
// means a closer surface.
std::array<float, WIDTH * HEIGHT> zBuffer;

// Resets every depth before drawing a new frame.
// The maximum value lets the first visible surface
// pass the depth test.
void clear_zbuffer()
{
    zBuffer.fill(std::numeric_limits<float>::max());
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
    const BoundingBox box{
        chapter_7_clampi(std::min({ x0, x1, x2 }), 0, WIDTH - 1),
        chapter_7_clampi(std::min({ y0, y1, y2 }), 0, HEIGHT - 1),
        chapter_7_clampi(std::max({ x0, x1, x2 }), 0, WIDTH - 1),
        chapter_7_clampi(std::max({ y0, y1, y2 }), 0, HEIGHT - 1)
    };

    // Compute 1 / area once, so we don't redo
    // the same division for every pixel.
    const float inverseArea = 1.0f / static_cast<float>(area);

    // Walk every pixel of the bounding box.
    for (int y = box.minY; y <= box.maxY; ++y)
    {
        for (int x = box.minX; x <= box.maxX; ++x)
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
            }.to_rgb();
        }
    }
}


// The anonymous namespace keeps it private to this chapter, like static does for a function.
namespace
{
    // What we keep of a vertex once it is projected on the screen.
    struct Chapter8ProjectedVertex
    {
        int x = 0;          // Horizontal position on the screen.
        int y = 0;          // Vertical position on the screen.
        float depth = 0.0f; // Depth after projection.
        bool visible = false; // True if the vertex is in front of the camera.
    };
}

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
        math::Mat4<float>::perspective(
            fovY, aspect, 0.1f, 100.0f);

    // View matrix.
    // The camera sits at (0, 0, 3), looks at the origin
    // and uses the Y axis as up.
    const math::Mat4<float> view =
        math::Mat4<float>::look_at(
            math::Vector3<float>(0.0f, 0.0f, 3.0f),
            math::Vector3<float>(0.0f, 0.0f, 0.0f),
            math::Vector3<float>(0.0f, 1.0f, 0.0f));

    // Rotation around the Y axis.
    // The angle grows with time, which animates the model.
    const math::Mat4<float> model =
        math::Mat4<float>::rotate(
            math::Quaternion::from_axis_angle(
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
            modelView.multiply_point_affine(mesh.vertices[i]);

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
            projection.multiply_point(cameraPosition);

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

