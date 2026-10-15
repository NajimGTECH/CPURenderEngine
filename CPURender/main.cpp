#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "Pixel.h"

#include <stdlib.h>
#include <iostream>
#include <algorithm>

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
                // Calculate normalized barycentric weights.
                const float weightA = edgeA / area;
                const float weightB = edgeB / area;
                const float weightC = edgeC / area;

                // Interpolate the RGB channels.
                const float red = weightA * pA.color.r + weightB * pB.color.r + weightC * pC.color.r;
                const float green = weightA * pA.color.g + weightB * pB.color.g + weightC * pC.color.g;
                const float blue = weightA * pA.color.b + weightB * pB.color.b + weightC * pC.color.b;

                // Clamp and round the channels to the valid RGB range.
                point.color.r = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(red)), 0, 255));
                point.color.g = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(green)), 0, 255));
                point.color.b = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(blue)), 0, 255));

                add_pixel_to_buffer(point);
            }
        }
    }
}

// =============
// Renderer custom functions

void update_render(SDL_Renderer* renderer, SDL_Texture* texture)
{
    //SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Drawing stuff...
    Pixel pA{ 100, 300, Color3{255, 0, 0} };
    Pixel pB{ 200, 100, Color3{0, 255, 0} };
    Pixel pC{ 300, 300, Color3{0, 0, 255} };
    chapter_3(pA, pB, pC);

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