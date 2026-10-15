#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "Pixel.h"

#include <stdlib.h>
#include <iostream>

// SDL Exercices

constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 600;

// Test pixels
Pixel pixel1{ 0.0f, 0.0f };
Pixel pixel2{ 400.0f, 300.0f };

void update_render(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderClear(renderer);

    // Drawing stuff...
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    px::drawLine(renderer, pixel1, pixel2);

    SDL_RenderPresent(renderer);
}

int main()
{
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

    SDL_Event event;

    bool isRunning = true;
    while (isRunning)
    {
        if (SDL_PollEvent(&event))
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

        update_render(renderer);
    }

    SDL_Quit();

    return 0;
}