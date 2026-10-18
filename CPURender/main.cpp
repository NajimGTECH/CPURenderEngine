#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "Pixel.h"
#include "Vec3.h"
#include "Mat4.h"
#include "Mesh.h"

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
void chapter_5_draw_line(const Pixel& from, const Pixel& to)
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

// Cube: 8 corners, and 12 edges joining two corners by index.
const math::Vector3<float> CHAPTER_5_CUBE_VERTICES[8] =
{
    { -1.0f, -1.0f, -1.0f },
    {  1.0f, -1.0f, -1.0f },
    {  1.0f,  1.0f, -1.0f },
    { -1.0f,  1.0f, -1.0f },

    { -1.0f, -1.0f,  1.0f },
    {  1.0f, -1.0f,  1.0f },
    {  1.0f,  1.0f,  1.0f },
    { -1.0f,  1.0f,  1.0f }
};

struct CubeEdge
{
    int start = 0, end = 0;
    int axis = 0; // 0 = X, 1 = Y, 2 = Z (the axis the edge is parallel to, used for its color)
};

const CubeEdge CHAPTER_5_CUBE_EDGES[12] =
{
    { 0, 1, 0 }, { 3, 2, 0 }, { 7, 6, 0 }, { 4, 5, 0 },
    { 0, 3, 1 }, { 1, 2, 1 }, { 5, 6, 1 }, { 4, 7, 1 },
    { 0, 4, 2 }, { 1, 5, 2 }, { 2, 6, 2 }, { 3, 7, 2 }
};

const Color3 CHAPTER_5_AXIS_COLORS[3] =
{
    Color3{ 230, 80, 80 },
    Color3{ 80, 210, 90 },
    Color3{ 90, 140, 240 }
};

// Rotation around X (row-vector convention).
math::Mat4<float> chapter_5_rotation_x(float angle)
{
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    return math::Mat4<float>(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c, s, 0.0f,
        0.0f, -s, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

// Rotation around Y (row-vector convention).
math::Mat4<float> chapter_5_rotation_y(float angle)
{
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    return math::Mat4<float>(
        c, 0.0f, s, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -s, 0.0f, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

struct ScreenVertex
{
    int x = 0, y = 0;
    bool visible = false; // false if behind the camera
};

// Model space -> clip space (p * MVP) -> NDC (divide by w) -> screen pixels (viewport).
ScreenVertex chapter_5_project_vertex(const math::Vector3<float>& vertex, const math::Mat4<float>& mvp)
{
    // Clip space. z is not needed for a wireframe, only x, y and w.
    const float clipX = vertex.x * mvp(0, 0) + vertex.y * mvp(1, 0) + vertex.z * mvp(2, 0) + mvp(3, 0);
    const float clipY = vertex.x * mvp(0, 1) + vertex.y * mvp(1, 1) + vertex.z * mvp(2, 1) + mvp(3, 1);
    const float clipW = vertex.x * mvp(0, 3) + vertex.y * mvp(1, 3) + vertex.z * mvp(2, 3) + mvp(3, 3);

    // w is the distance in front of the camera: <= 0 means behind it (and we can't divide by 0)
    if (clipW <= 0.0001f) return {};

    // Perspective divide: far points get divided by a bigger w, so they end up closer to the center
    const float ndcX = clipX / clipW;
    const float ndcY = clipY / clipW;

    // Viewport: NDC [-1, 1] -> pixels (Y is flipped: up in NDC is down on screen)
    return {
        static_cast<int>((ndcX + 1.0f) * 0.5f * WIDTH),
        static_cast<int>((1.0f - ndcY) * 0.5f * HEIGHT),
        true
    };
}

// Spinning wireframe cube through the full 3D pipeline: Model -> View -> Projection.
void chapter_5(float seconds)
{
    // Model: rotate the cube around its X and Y axes
    const math::Mat4<float> model = chapter_5_rotation_x(seconds * 0.5f) * chapter_5_rotation_y(seconds * 0.8f);

    // View: camera at (0, 0, 5), looking at the origin, +Y is up
    const math::Mat4<float> view = math::Mat4<float>::LookAt(
        math::Vector3<float>(0.0f, 0.0f, 5.0f),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f)
    );

    // Projection: 60 degrees vertical field of view
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 60.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(fovY, aspect, 0.1f, 100.0f);

    // Row vectors (p * M): model, then view, then projection
    const math::Mat4<float> mvp = model * view * projection;

    // Project the 8 corners once, the edges reuse them by index
    ScreenVertex screenVertices[8];
    for (int i = 0; i < 8; i++)
    {
        screenVertices[i] = chapter_5_project_vertex(CHAPTER_5_CUBE_VERTICES[i], mvp);
    }

    for (const CubeEdge& edge : CHAPTER_5_CUBE_EDGES)
    {
        const ScreenVertex& start = screenVertices[edge.start];
        const ScreenVertex& end = screenVertices[edge.end];
        if (!start.visible || !end.visible) continue;

        const Color3& color = CHAPTER_5_AXIS_COLORS[edge.axis];
        chapter_5_draw_line(Pixel{ start.x, start.y, color }, Pixel{ end.x, end.y, color });
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

        chapter_5_draw_line(pA, pB);
        chapter_5_draw_line(pB, pC);
        chapter_5_draw_line(pC, pA);
    }
}

// =============
// Renderer custom functions

void update_render(SDL_Renderer* renderer, SDL_Texture* texture)
{
    //SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Drawing stuff...
    Pixel pA{WIDTH / 2, HEIGHT / 2 - CIRCLE_RAY, Color3{255, 0, 0}};
    Pixel pB{ WIDTH / 2 - CIRCLE_RAY, HEIGHT / 2 + CIRCLE_RAY, Color3{0, 255, 0} };
    Pixel pC{ WIDTH / 2 + CIRCLE_RAY, HEIGHT / 2 + CIRCLE_RAY, Color3{0, 0, 255} };
    chapter_3(pA, pB, pC);

    chapter_4_draw_circle(Pixel{ static_cast<int>(WIDTH / 2), static_cast<int>(HEIGHT / 2), Color3{255, 255, 255} }, CIRCLE_RAY);

    SDL_UpdateTexture(texture, 0, frameBuffer, WIDTH * sizeof(uint32_t));
    SDL_RenderTexture(renderer, texture, 0, 0);
    SDL_RenderPresent(renderer);
}

// =============
// Main function

int main()
{
    // Load the mesh once, before the loop (chapter 6)
    Mesh spot;
    if (!mesh::loadObj("assets/spot.obj", spot)) return EXIT_FAILURE;
    std::cout << "Loaded spot.obj:\t" << spot.vertices.size() << " vertices, " << spot.faces.size() << " faces\n";

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

        // Clear the CPU framebuffer.
        const uint32_t backgroundColor =
            Color3{ 16, 16, 24, 255 }.toRGB();

        std::fill_n(frameBuffer, WIDTH * HEIGHT, backgroundColor);

        // Draw the current chapter.
        //chapter_4(seconds);
        //chapter_5(seconds);
        chapter_6(spot, seconds);

        // Upload the framebuffer and display it.
        SDL_RenderClear(renderer);

        SDL_UpdateTexture(
            texture,
            nullptr,
            frameBuffer,
            WIDTH * sizeof(uint32_t)
        );

        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

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