#include <SDL3/SDL.h>

#include "Pixel.h"
#include "Vec3.h"
#include "Mat4.h"

#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <cmath>

// Window dimensions
constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

// Framebuffer
uint32_t frameBuffer[WIDTH * HEIGHT];

constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 600;
constexpr int CIRCLE_RAY = 200;

// Cube rotation speed
constexpr float ROTATION_SPEED = 0.1f;

// 2D projected point
struct Vector2
{
    int x;
    int y;
};

// Cube vertices in 3D world space
const Vector3 vertices[8] =
{
    { -1.0f, -1.0f, 4.0f },
    {  1.0f, -1.0f, 4.0f },
    {  1.0f,  1.0f, 4.0f },
    { -1.0f,  1.0f, 4.0f },

    { -1.0f, -1.0f, 6.0f },
    {  1.0f, -1.0f, 6.0f },
    {  1.0f,  1.0f, 6.0f },
    { -1.0f,  1.0f, 6.0f }
};

// Cube edges defined by pairs of vertex indices
const int edges[12][2] =
{
    { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
    { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
    { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
};

// Add a pixel to the framebuffer
void add_pixel_to_buffer(const Pixel& px)
{
    if (px.x < 0 || px.x >= WIDTH ||
        px.y < 0 || px.y >= HEIGHT)
    {
        return;
    }

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

    const int dx = std::abs(end.x - start.x);
    const int dy = std::abs(end.y - start.y);

    const int stepX = (start.x < end.x) ? 1 : -1;
    const int stepY = (start.y < end.y) ? 1 : -1;

    int error = dx - dy;

    while (true)
    {
        add_pixel_to_buffer(Pixel(x, y, start.color));

        if (x == end.x && y == end.y)
        {
            break;
        }

        const int doubleError = 2 * error;

        if (doubleError > -dy)
        {
            error -= dy;
            x += stepX;
        }

        if (doubleError < dx)
        {
            error += dx;
            y += stepY;
        }
    }
}

// Project a 3D camera-space point onto the 2D screen
Vector2 ProjectPoint(const Vector3& point, float focalLength)
{
    // Ignore points behind or too close to the camera
    if (point.z <= 0.001f)
    {
        return { -1, -1 };
    }

    const float screenX =
        WIDTH / 2.0f + focalLength * point.x / point.z;

    const float screenY =
        HEIGHT / 2.0f - focalLength * point.y / point.z;

    return
    {
        static_cast<int>(screenX),
        static_cast<int>(screenY)
    };
}

Vector3 RotatePoint(const Vector3& point, float yaw, float pitch)
{

    const float cosYaw = std::cos(yaw);
    const float sinYaw = std::sin(yaw);

    const float rotatedX = cosYaw * point.x + sinYaw * point.z;
    const float rotatedZ = -sinYaw * point.x + cosYaw * point.z;

 
    const float cosPitch = std::cos(pitch);
    const float sinPitch = std::sin(pitch);

    const float rotatedY = cosPitch * point.y - sinPitch * rotatedZ;
    const float finalZ = sinPitch * point.y + cosPitch * rotatedZ;

    return { rotatedX, rotatedY, finalZ };
}

// Chapter 1: Color gradient
void chapter_1()
{
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            const float red = static_cast<float>(x) / WIDTH;
            const float green = static_cast<float>(y) / HEIGHT;

            const uint8_t r = static_cast<uint8_t>(red * 255);
            const uint8_t g = static_cast<uint8_t>(green * 255);
            const uint8_t b = 100;

            frameBuffer[y * WIDTH + x] =
                (static_cast<uint32_t>(r) << 16) |
                (static_cast<uint32_t>(g) << 8) |
                static_cast<uint32_t>(b);
        }
    }
}

// Chapter 2: Radial starburst
void chapter_2()
{
    const float centerX = WIDTH / 2.0f;
    const float centerY = HEIGHT / 2.0f;
    const int rayCount = 36;
    const float maxRadius = static_cast<float>(
        std::sqrt(WIDTH * WIDTH + HEIGHT * HEIGHT));

    // Clear the framebuffer
    std::fill(
        std::begin(frameBuffer),
        std::end(frameBuffer),
        0x101018
    );

    for (int i = 0; i < rayCount; ++i)
    {
        const float angle =
            2.0f * std::numbers::pi_v<float> *i / rayCount;

        const int endX = static_cast<int>(
            centerX + std::cos(angle) * maxRadius);

        const int endY = static_cast<int>(
            centerY + std::sin(angle) * maxRadius);

        const Pixel start(
            static_cast<int>(centerX),
            static_cast<int>(centerY),
            Color3(255, 255, 255)
        );

        const Pixel end(
            endX,
            endY,
            Color3(80, 180, 255)
        );

        add_line_to_buffer(start, end);
    }
}

// Chapter 5: 3D wireframe cube projection
void chapter_5()
{
    // Clear the framebuffer with a dark background
    std::fill(
        std::begin(frameBuffer),
        std::end(frameBuffer),
        0x101018
    );

    // Field of view in degrees
    constexpr float FOV_DEGREES = 70.0f;

    const float fovRadians =
        FOV_DEGREES * std::numbers::pi_v<float> / 180.0f;

    const float focalLength =
        (HEIGHT / 2.0f) / std::tan(fovRadians / 2.0f);

    // Transform and project all cube vertices
    Vector2 projectedVertices[8];

    for (int i = 0; i < 8; ++i)
    {
        const Vector3 localPoint =
        {
            vertices[i].x,
            vertices[i].y,
            vertices[i].z - 5.0f
        };

        
        const Vector3 rotatedPoint =
            RotatePoint(localPoint, cubeYaw, cubePitch);

        
        const Vector3 worldPoint =
        {
            rotatedPoint.x,
            rotatedPoint.y,
            rotatedPoint.z + 5.0f
        };

       
        const Vector3 cameraPoint =
            WorldToCamera(worldPoint, camera);

        projectedVertices[i] =
            ProjectPoint(cameraPoint, focalLength);
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

        const Pixel startPixel(start.x, start.y, edgeColor);
        const Pixel endPixel(end.x, end.y, edgeColor);

        add_line_to_buffer(startPixel, endPixel);
    }
}

// Update and display the framebuffer
void update_render(SDL_Renderer* renderer, SDL_Texture* texture)
{
    chapter_5();

    // Drawing stuff...
    Pixel pA{WIDTH / 2, HEIGHT / 2 - CIRCLE_RAY, Color3{255, 0, 0}};
    Pixel pB{ WIDTH / 2 - CIRCLE_RAY, HEIGHT / 2 + CIRCLE_RAY, Color3{0, 255, 0} };
    Pixel pC{ WIDTH / 2 + CIRCLE_RAY, HEIGHT / 2 + CIRCLE_RAY, Color3{0, 0, 255} };
    chapter_3(pA, pB, pC);

    chapter_4_draw_circle(Pixel{ static_cast<int>(WIDTH / 2), static_cast<int>(HEIGHT / 2), Color3{255, 255, 255} }, CIRCLE_RAY);

    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL initialization failed: "
            << SDL_GetError() << '\n';
        return EXIT_FAILURE;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Chapter 5 - Projection",
        WIDTH,
        HEIGHT,
        0
    );

    if (window == nullptr)
    {
        std::cerr << "Window creation failed: "
            << SDL_GetError() << '\n';

        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    if (renderer == nullptr)
    {
        std::cerr << "Renderer creation failed: "
            << SDL_GetError() << '\n';

        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH,
        HEIGHT
    );

    if (texture == nullptr)
    {
        std::cerr << "Texture creation failed: "
            << SDL_GetError() << '\n';

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    bool isRunning = true;
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
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    camera.isLooking = true;
                    SDL_SetWindowRelativeMouseMode(window, true);
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    camera.isLooking = false;
                    SDL_SetWindowRelativeMouseMode(window, false);
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                if (camera.isLooking)
                {
                    camera.yaw +=
                        event.motion.xrel * MOUSE_SENSITIVITY;

                    camera.pitch -=
                        event.motion.yrel * MOUSE_SENSITIVITY;

                    camera.pitch = std::clamp(
                        camera.pitch,
                        -MAX_PITCH,
                        MAX_PITCH
                    );
                }
            }
        }

        // Calculate the rotation angle from elapsed time.
        const double seconds =
            static_cast<double>(SDL_GetPerformanceCounter() - start)
            / static_cast<double>(freq);

        const float angle = static_cast<float>(seconds);

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

        // Clear the CPU framebuffer.
        const uint32_t backgroundColor =
            Color3{ 16, 16, 24, 255 }.toRGB();

        std::fill_n(frameBuffer, WIDTH * HEIGHT, backgroundColor);

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
    }

    SDL_SetWindowRelativeMouseMode(window, false);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return EXIT_SUCCESS;
}