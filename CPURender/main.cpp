#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "Pixel.h"
#include "Vec3.h"
#include "Mat4.h"
#include "Mesh.h"
#include "Texture.h"

#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <limits>
#include <numbers>

// ==============
// SDL Exercices
// ==============

// =============
// Screen

constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 600;
constexpr int CIRCLE_RAY = 200;

// =============
// Buffer

uint32_t frameBuffer[WIDTH * HEIGHT];

// Depth of the closest thing drawn so far on each pixel (chapter 9).
float depthBuffer[WIDTH * HEIGHT];

void add_pixel_to_buffer(Pixel px)
{
    if (px.x < 0 || px.x >= WIDTH || px.y < 0 || px.y >= HEIGHT) { SDL_Log("Pixel out of screen"); return; }

    frameBuffer[px.y * WIDTH + px.x] = px.color.toRGB();
}

// =============
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

// =============
// Chapters

void chapter_1()
{
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            Pixel pixel(x, y, Color3{ static_cast<uint8_t>(x * 255 / WIDTH), 64, static_cast<uint8_t>(y * 255 / WIDTH) });
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
            float edgeC= chapter_3_edge(pA, pB, point);

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

// =============
// Renderer custom functions

// Clears the frame, draws the current chapter, then sends the framebuffer to the screen.
void update_render(SDL_Renderer* renderer, SDL_Texture* texture, float seconds, const Mesh& mesh, const Texture& meshTexture)
{
    // Clear the CPU framebuffer.
    std::fill_n(frameBuffer, WIDTH * HEIGHT, Color3{ 16, 16, 24, 255 }.toRGB());

    // Draw the current chapter.
    //chapter_1();
    //chapter_4(seconds);
    //chapter_6(mesh, seconds);
    chapter_9(mesh, meshTexture, seconds);

    // Upload the framebuffer and display it.
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(texture, nullptr, frameBuffer, WIDTH * sizeof(uint32_t));
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

// =============
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

    // Init
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        SDL_Log("Couldn't init SDL\n");
        return EXIT_FAILURE;
    }

    SDL_Window* window = SDL_CreateWindow("Fenetre Test", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
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
            }
        }

        // Elapsed time, used by the chapters to animate.
        const float seconds = static_cast<float>(
            static_cast<double>(SDL_GetPerformanceCounter() - start)
            / static_cast<double>(freq));

        // Draw the frame (the current chapter is picked inside update_render).
        update_render(renderer, texture, seconds, spot, spotTexture);

        // FPS in the window title, refreshed once per second.
        const Uint64 now = SDL_GetPerformanceCounter();
        fps_accum += static_cast<double>(now - last) / static_cast<double>(freq);
        fps_count++;
        last = now;

        if (fps_accum >= 1.0)
        {
            const double fps = fps_count / fps_accum;
            const double ms = 1000.0 * fps_accum / fps_count;
            SDL_snprintf(title, sizeof(title), "Fenetre Test | %d tris | %.1f FPS (%.2f ms)", static_cast<int>(spot.faces.size()), fps, ms);
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