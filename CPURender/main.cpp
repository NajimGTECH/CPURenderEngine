#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdint>

// ============================================================
// Constants
// ============================================================

namespace
{
    constexpr int WINDOW_WIDTH = 800;
    constexpr int WINDOW_HEIGHT = 600;

    constexpr std::uint32_t FRAMEBUFFER_SIZE =
        WINDOW_WIDTH * WINDOW_HEIGHT;

    // ========================================================
    // Framebuffer
    // ========================================================

    std::uint32_t framebuffer[FRAMEBUFFER_SIZE];

    // ========================================================
    // Pixel Utilities
    // ========================================================

    // Packs RGBA channels into an ARGB8888 pixel.
    std::uint32_t RGB(std::uint8_t r, std::uint8_t g, std::uint8_t b)
    {
        return (0xFFu << 24)
            | (static_cast<std::uint32_t>(r) << 16)
            | (static_cast<std::uint32_t>(g) << 8)
            | static_cast<std::uint32_t>(b);
    }

    // Writes a pixel if its coordinates are inside the framebuffer.
    void PutPixel(int x, int y, std::uint32_t color)
    {
        if (x < 0 || x >= WINDOW_WIDTH ||
            y < 0 || y >= WINDOW_HEIGHT)
        {
            return;
        }

        framebuffer[y * WINDOW_WIDTH + x] = color;
    }

    // Computes the gradient for the entire framebuffer.
    void DrawGradient()
    {
        for (int y = 0; y < WINDOW_HEIGHT; ++y)
        {
            for (int x = 0; x < WINDOW_WIDTH; ++x)
            {
                const auto r = static_cast<std::uint8_t>(
                    x * 255 / WINDOW_WIDTH);

                const auto b = static_cast<std::uint8_t>(
                    y * 255 / WINDOW_HEIGHT);

                PutPixel(x, y, RGB(r, 64, b));
            }
        }
    }
}

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    // Initialize SDL.
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    // Create the window.
    SDL_Window* window = SDL_CreateWindow(
        "Chapter 01 - Framebuffer",
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0);

    if (window == nullptr)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create the renderer.
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    if (renderer == nullptr)
    {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create a streaming texture to display the framebuffer.
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WINDOW_WIDTH,
        WINDOW_HEIGHT);

    if (texture == nullptr)
    {
        SDL_Log("SDL_CreateTexture failed: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;

    while (running)
    {
        // Handle window and keyboard events.
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE)
                {
                    running = false;
                }
                break;

            default:
                break;
            }
        }

        // Compute the gradient.
        DrawGradient();

        // Upload the framebuffer to the texture.
        if (!SDL_UpdateTexture(
            texture,
            nullptr,
            framebuffer,
            WINDOW_WIDTH * static_cast<int>(sizeof(std::uint32_t))))
        {
            SDL_Log("SDL_UpdateTexture failed: %s", SDL_GetError());
            running = false;
            continue;
        }

        // Display the texture.
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    // Release SDL resources in reverse creation order.
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}