#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "Pixel.h"

#include <stdlib.h>
#include <iostream>

#include <cmath>
#include <algorithm>
#include <numbers>

// ==============
// SDL Exercices
// ==============

// =============
// Screen

constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 600;

// =============
// Buffer

uint32_t frameBuffer[WIDTH * HEIGHT];

void add_pixel_to_buffer(Pixel px)
{
    if (px.x < 0 || px.x >= WIDTH || px.y < 0 || px.y >= HEIGHT) { SDL_Log("Pixel out of screen"); return; }

    frameBuffer[px.y * WIDTH + px.x] = px.color.toRGB();
}

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

Color3 LerpColor(const Color3& start, const Color3& end, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);

    return Color3(
        static_cast<uint8_t>(start.r + (static_cast<float>(end.r) - start.r) * t),
        static_cast<uint8_t>(start.g + (static_cast<float>(end.g) - start.g) * t),
        static_cast<uint8_t>(start.b + (static_cast<float>(end.b) - start.b) * t)
    );
}

Color3 GetRayColor(float angleDegrees)
{
    const Color3 pink(255, 65, 210);
    const Color3 mint(40, 255, 190);
    const Color3 blue(20, 75, 255);
    const Color3 purple(220, 0, 255);

    if (angleDegrees < 90.0f)
    {
        return LerpColor(pink, mint, angleDegrees / 90.0f);
    }

    if (angleDegrees < 180.0f)
    {
        return LerpColor(mint, blue, (angleDegrees - 90.0f) / 90.0f);
    }

    if (angleDegrees < 270.0f)
    {
        return LerpColor(blue, purple, (angleDegrees - 180.0f) / 90.0f);
    }

    return LerpColor(purple, pink, (angleDegrees - 270.0f) / 90.0f);
}


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

void chapter_2()
{
   
    const Color3 background(15, 15, 23);

    for (int i = 0; i < WIDTH * HEIGHT; ++i)
    {
        frameBuffer[i] = background.toRGB();
    }


    const float centerX = WIDTH / 2.0f;
    const float centerY = HEIGHT / 2.0f;

   
    constexpr int RAY_COUNT = 64;
    constexpr float RADIUS = 280.0f;
	constexpr double PI = std::numbers::pi_v<double>;

 
    for (int i = 0; i < RAY_COUNT; ++i)
    {
       
        const float angle = (2.0f * PI * i) / RAY_COUNT;

        
        const float angleDegrees = angle * 180.0f / PI;

       
        const int endX = static_cast<int>(
            centerX + std::cos(angle) * RADIUS
            );

        const int endY = static_cast<int>(
            centerY + std::sin(angle) * RADIUS
            );

      
        const Color3 rayColor = GetRayColor(angleDegrees);

       
        const Pixel start(
            static_cast<int>(centerX),
            static_cast<int>(centerY),
            rayColor
        );

        const Pixel end(endX, endY, rayColor);

        add_line_to_buffer(start, end);
    }

   
    add_pixel_to_buffer(
        Pixel(
            static_cast<int>(centerX),
            static_cast<int>(centerY),
            Color3(255, 255, 255)
        )
    );
}


// =============
// Renderer custom functions

void update_render(SDL_Renderer* renderer, SDL_Texture* texture)
{
    //SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Drawing stuff...
    chapter_2();

    SDL_UpdateTexture(texture, 0, frameBuffer, WIDTH * sizeof(uint32_t));
    SDL_RenderTexture(renderer, texture, 0, 0);
    SDL_RenderPresent(renderer);
}

// =============
// Main function

int main()
{
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

        update_render(renderer, texture);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}