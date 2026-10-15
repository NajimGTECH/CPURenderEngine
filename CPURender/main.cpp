#include <SDL3/SDL.h>

#include "Pixel.h"
#include "Camera.h"

#include <cstdlib>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numbers>

// Window dimensions
constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

// Framebuffer
uint32_t frameBuffer[WIDTH * HEIGHT];

// Camera
Camera camera;
float cubeYaw = 0.0f;
float cubePitch = 0.0f;

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

// Draw a line using Bresenham's algorithm
void add_line_to_buffer(const Pixel& start, const Pixel& end)
{
    int x = start.x;
    int y = start.y;

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

    // Draw the 12 edges of the cube
    for (int i = 0; i < 12; ++i)
    {
        const int startIndex = edges[i][0];
        const int endIndex = edges[i][1];

        const Vector2 start = projectedVertices[startIndex];
        const Vector2 end = projectedVertices[endIndex];

        // Skip edges with vertices behind the camera
        if (start.x == -1 || end.x == -1)
        {
            continue;
        }

        Color3 edgeColor;

        if (i < 4)
        {
            edgeColor = Color3(255, 80, 80);
        }
        else if (i < 8)
        {
            edgeColor = Color3(80, 160, 255);
        }
        else
        {
            edgeColor = Color3(100, 255, 140);
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

    SDL_UpdateTexture(
        texture,
        nullptr,
        frameBuffer,
        WIDTH * static_cast<int>(sizeof(uint32_t))
    );

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

        cubeYaw += ROTATION_SPEED * 0.016f;
        cubePitch += ROTATION_SPEED * 0.5f * 0.016f;

        update_render(renderer, texture);
    }

    SDL_SetWindowRelativeMouseMode(window, false);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return EXIT_SUCCESS;
}