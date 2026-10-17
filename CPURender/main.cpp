
#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "Pixel.h"
#include "Vec3.h"
#include "Mat4.h"

namespace
{
// Configuration
    constexpr int WIDTH = 800;
    constexpr int HEIGHT = 600;

    constexpr float FIELD_OF_VIEW = 1.0471975512f;
    constexpr float NEAR_PLANE = 0.1f;
    constexpr float FAR_PLANE = 100.0f;
    constexpr float CAMERA_DISTANCE = 5.0f;

    constexpr float ROTATION_SPEED_X = 0.5f;
    constexpr float ROTATION_SPEED_Y = 0.8f;
    constexpr float MIN_CLIP_W = 0.0001f;

    using Vec3 = math::Vector3<float>;
    using Mat4 = math::Mat4<float>;
    using Color = std::uint32_t;

    constexpr Color MakeColor(
        std::uint8_t red,
        std::uint8_t green,
        std::uint8_t blue)
    {
        return 0xFF000000u
            | (static_cast<Color>(red) << 16)
            | (static_cast<Color>(green) << 8)
            | static_cast<Color>(blue);
    }

    constexpr Color BACKGROUND_COLOR = MakeColor(16, 16, 24);

    constexpr std::array<Color, 3> AXIS_COLORS =
    {
        MakeColor(230, 80, 80),
        MakeColor(80, 210, 90),
        MakeColor(90, 140, 240)
    };

    std::array<Color, WIDTH* HEIGHT> frameBuffer{};

 
    // Cube geometry
   

    constexpr std::array<Vec3, 8> CUBE_VERTICES =
    { {
        Vec3(-1.0f, -1.0f, -1.0f),
        Vec3(1.0f, -1.0f, -1.0f),
        Vec3(1.0f,  1.0f, -1.0f),
        Vec3(-1.0f,  1.0f, -1.0f),

        Vec3(-1.0f, -1.0f,  1.0f),
        Vec3(1.0f, -1.0f,  1.0f),
        Vec3(1.0f,  1.0f,  1.0f),
        Vec3(-1.0f,  1.0f,  1.0f)
    } };

    struct CubeEdge
    {
        int start;
        int end;
        int axis;
    };

    constexpr std::array<CubeEdge, 12> CUBE_EDGES =
    { {
        { 0, 1, 0 }, { 3, 2, 0 },
        { 7, 6, 0 }, { 4, 5, 0 },

        { 0, 3, 1 }, { 1, 2, 1 },
        { 5, 6, 1 }, { 4, 7, 1 },

        { 0, 4, 2 }, { 1, 5, 2 },
        { 2, 6, 2 }, { 3, 7, 2 }
    } };

    struct ScreenVertex
    {
        int x = 0;
        int y = 0;
        bool visible = false;
    };

    // Framebuffer
   
    void ClearFrameBuffer(Color color)
    {
        frameBuffer.fill(color);
    }

    void PutPixel(int x, int y, Color color)
    {
        if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        {
            return;
        }

        frameBuffer[static_cast<std::size_t>(y) * WIDTH + x] = color;
    }

 
    // Line rasterization
  

    void DrawLine(
        int x0,
        int y0,
        int x1,
        int y1,
        Color color)
    {
        const int dx = std::abs(x1 - x0);
        const int dy = std::abs(y1 - y0);

        const int stepX = (x0 < x1) ? 1 : -1;
        const int stepY = (y0 < y1) ? 1 : -1;

        int error = dx - dy;

        for (;;)
        {
            PutPixel(x0, y0, color);

            if (x0 == x1 && y0 == y1)
            {
                break;
            }

            const int doubledError = 2 * error;

            if (doubledError > -dy)
            {
                error -= dy;
                x0 += stepX;
            }

            if (doubledError < dx)
            {
                error += dx;
                y0 += stepY;
            }
        }
    }

    
    // Rotation matrices
 

    Mat4 MakeRotationX(float angle)
    {
        const float cosine = std::cos(angle);
        const float sine = std::sin(angle);

        // Row-vector convention.
        return Mat4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, cosine, sine, 0.0f,
            0.0f, -sine, cosine, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

    Mat4 MakeRotationY(float angle)
    {
        const float cosine = std::cos(angle);
        const float sine = std::sin(angle);

        // Row-vector convention.
        return Mat4(
            cosine, 0.0f, sine, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            -sine, 0.0f, cosine, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

 
    // Projection
  

    ScreenVertex ProjectVertex(
        const Vec3& vertex,
        const Mat4& mvp)
    {
        // Transform the model-space position into clip space.
        const float clipX =
            vertex.x * mvp(0, 0)
            + vertex.y * mvp(1, 0)
            + vertex.z * mvp(2, 0)
            + mvp(3, 0);

        const float clipY =
            vertex.x * mvp(0, 1)
            + vertex.y * mvp(1, 1)
            + vertex.z * mvp(2, 1)
            + mvp(3, 1);

        const float clipW =
            vertex.x * mvp(0, 3)
            + vertex.y * mvp(1, 3)
            + vertex.z * mvp(2, 3)
            + mvp(3, 3);

        // Avoid dividing by zero or projecting a point behind the camera.
        if (clipW <= MIN_CLIP_W)
        {
            return {};
        }

        // Perspective divide: clip space to normalized device coordinates.
        const float ndcX = clipX / clipW;
        const float ndcY = clipY / clipW;

        // Viewport transform: normalized coordinates to pixel coordinates.
        const int screenX = static_cast<int>(
            (ndcX + 1.0f) * 0.5f * WIDTH
            );

        const int screenY = static_cast<int>(
            (1.0f - ndcY) * 0.5f * HEIGHT
            );

        return { screenX, screenY, true };
    }

  
    // Cube rendering
  

    void RenderCube(float elapsedSeconds)
    {
        ClearFrameBuffer(BACKGROUND_COLOR);

        // Model: rotate the cube around its local X and Y axes.
        const Mat4 rotationX =
            MakeRotationX(elapsedSeconds * ROTATION_SPEED_X);

        const Mat4 rotationY =
            MakeRotationY(elapsedSeconds * ROTATION_SPEED_Y);

        const Mat4 model = rotationX * rotationY;

        // View: place the camera at (0, 0, 5), looking at the origin.
        const Mat4 view = Mat4::LookAt(
            Vec3(0.0f, 0.0f, CAMERA_DISTANCE),
            Vec3(0.0f, 0.0f, 0.0f),
            Vec3(0.0f, 1.0f, 0.0f)
        );

     
        const float aspectRatio =
            static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);

        const Mat4 projection = Mat4::Perspective(
            FIELD_OF_VIEW,
            aspectRatio,
            NEAR_PLANE,
            FAR_PLANE
        );

        // Row-vector convention: model, then view, then projection.
        const Mat4 mvp = model * view * projection;

        // Transform the eight cube vertices only once per frame.
        std::array<ScreenVertex, 8> screenVertices{};

        for (std::size_t i = 0; i < CUBE_VERTICES.size(); ++i)
        {
            screenVertices[i] = ProjectVertex(CUBE_VERTICES[i], mvp);
        }

        // Draw the twelve edges using the colour associated with each axis.
        for (const CubeEdge& edge : CUBE_EDGES)
        {
            const ScreenVertex& start = screenVertices[edge.start];
            const ScreenVertex& end = screenVertices[edge.end];

            if (!start.visible || !end.visible)
            {
                continue;
            }

            DrawLine(
                start.x,
                start.y,
                end.x,
                end.y,
                AXIS_COLORS[edge.axis]
            );
        }
    }

   
    // SDL presentation
  

    bool PresentFrame(SDL_Renderer* renderer, SDL_Texture* texture)
    {
        if (!SDL_UpdateTexture(
            texture,
            nullptr,
            frameBuffer.data(),
            WIDTH * static_cast<int>(sizeof(Color))))
        {
            SDL_Log("SDL_UpdateTexture failed: %s", SDL_GetError());
            return false;
        }

        if (!SDL_RenderClear(renderer))
        {
            SDL_Log("SDL_RenderClear failed: %s", SDL_GetError());
            return false;
        }

        if (!SDL_RenderTexture(renderer, texture, nullptr, nullptr))
        {
            SDL_Log("SDL_RenderTexture failed: %s", SDL_GetError());
            return false;
        }

        SDL_RenderPresent(renderer);
        return true;
    }
}


// Main


int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Chapter 05 - 3D pipeline",
        WIDTH,
        HEIGHT,
        SDL_WINDOW_RESIZABLE
    );

    if (window == nullptr)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    if (renderer == nullptr)
    {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
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
        SDL_Log("SDL_CreateTexture failed: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    std::cout << "Initialized renderer: "
        << SDL_GetRendererName(renderer) << '\n';

    const Uint64 frequency = SDL_GetPerformanceFrequency();
    const Uint64 startTime = SDL_GetPerformanceCounter();

    Uint64 lastFrameTime = startTime;
    double fpsAccumulator = 0.0;
    int fpsFrameCount = 0;

    bool isRunning = true;

    while (isRunning)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                isRunning = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN
                && event.key.key == SDLK_ESCAPE)
            {
                isRunning = false;
            }
        }

        if (!isRunning)
        {
            break;
        }

        const Uint64 currentTime = SDL_GetPerformanceCounter();

        const float elapsedSeconds = static_cast<float>(
            static_cast<double>(currentTime - startTime)
            / static_cast<double>(frequency)
            );

        RenderCube(elapsedSeconds);

        if (!PresentFrame(renderer, texture))
        {
            isRunning = false;
            break;
        }

        // Update the FPS counter approximately once per second.
        const Uint64 frameEndTime = SDL_GetPerformanceCounter();

        const double deltaTime =
            static_cast<double>(frameEndTime - lastFrameTime)
            / static_cast<double>(frequency);

        lastFrameTime = frameEndTime;
        fpsAccumulator += deltaTime;
        ++fpsFrameCount;

        if (fpsAccumulator >= 1.0)
        {
            const double fps =
                static_cast<double>(fpsFrameCount) / fpsAccumulator;

            const double millisecondsPerFrame =
                1000.0 * fpsAccumulator / fpsFrameCount;

            char title[160];

            std::snprintf(
                title,
                sizeof(title),
                "Chapter 05 - 3D pipeline | %.1f FPS (%.2f ms/frame)",
                fps,
                millisecondsPerFrame
            );

            SDL_SetWindowTitle(window, title);

            fpsAccumulator = 0.0;
            fpsFrameCount = 0;
        }
    }

    // Destroy resources in reverse creation order.
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
