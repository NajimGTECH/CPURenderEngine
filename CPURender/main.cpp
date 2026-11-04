#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "ECS.h"
#include "Framebuffer.h"
#include "Scene.h"
#include "Systems.h"
#include "Chapters/Chapters.h"

#include <stdlib.h>
#include <iostream>
#include <algorithm>

// The application shell: opens the window, then runs the frame loop.
// Nothing here knows what is in the scene: it comes from Scene.cpp, and the systems drive it.
// See ARCHITECTURE.md at the root of the repository for the big picture.

// Main function

int main()
{
    // The assets are declared first: the entities point to them.
    Assets assets;
    if (!scene::load_assets(assets)) return EXIT_FAILURE;

    ECS ecs;
    const ComponentIDs ids = scene::create(ecs.get_entity_manager(), assets);

    // Init
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        SDL_Log("Couldn't init SDL\n");
        return EXIT_FAILURE;
    }

    SDL_Window* window = SDL_CreateWindow("Test Window", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
    if (window == nullptr) { SDL_Log("Window could not be created.\n"); return EXIT_FAILURE; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) { SDL_Log("Renderer could not be created.\n"); return EXIT_FAILURE; }
    std::cout << "Initialized renderer:\t" << SDL_GetRendererName(renderer) << '\n';

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (texture == nullptr) { SDL_Log("Texture could not be created.\n"); return EXIT_FAILURE; }

    // Elapsed time, written by the frame loop and read by the systems that animate.
    float seconds = 0.0f;   // since the start
    float deltaTime = 0.0f; // since the previous frame

    // One frame, in order. A System only receives the EntityManager: the rest comes through the lambda's captures.
    ecs.add_system([&](EntityManager& em) { systems::spin_system(em, ids, seconds); });
    ecs.add_system([&](EntityManager& em) { systems::orbit_system(em, ids, seconds); });
    ecs.add_system([&](EntityManager& em) { systems::light_sweep_system(em, ids, seconds); });
    ecs.add_system([&](EntityManager& em) { systems::particle_system(em, ids, deltaTime); });

    // Clear the CPU framebuffer.
    ecs.add_system([](EntityManager&) {
        std::fill_n(frameBuffer, WIDTH * HEIGHT, Color3{ 16, 16, 24, 255 }.to_rgb());
        });

    // Draw the current chapter. The ECS scene is the default merged result.
    // To go back to an older chapter (Chapters/), uncomment its call and comment out systems::render.
    ecs.add_system([&](EntityManager& em) {
        int drawnTriangles = 0;
        int culledTriangles = 0;
        //chapter_8(assets.spot, seconds, drawnTriangles, culledTriangles);
        //chapter_9(assets.spot, assets.spotTexture, seconds);
        //chapter_10(assets.spot, assets.spotTexture, assets.spot.faceNormals, seconds);
        //chapter_11(assets.spot, assets.spotTexture, assets.spot.vertexNormals, seconds);
        //chapter_12(assets.spot, assets.spotTexture, assets.spot.vertexNormals, seconds);

        // Shaders, materials and particles (keys 1, 2 and 3 for the lights, T for transparency, P for particles)
        systems::render(em, ids);
        });

    // Upload the CPU framebuffer, then draw the text on top.
    ecs.add_system([&](EntityManager&) {
        SDL_RenderClear(renderer);
        SDL_UpdateTexture(texture, nullptr, frameBuffer, WIDTH * sizeof(uint32_t));
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        });
    ecs.add_system([&](EntityManager& em) { systems::draw_hud(em, ids, renderer); });
    ecs.add_system([&](EntityManager&) { SDL_RenderPresent(renderer); });

    SDL_Event event;

    // FPS measurement
    const Uint64 freq = SDL_GetPerformanceFrequency();
    const Uint64 start = SDL_GetPerformanceCounter();
    Uint64 last = start;
    Uint64 previousFrame = start;
    double fps_accum = 0.0;
    int    fps_count = 0;

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
                // Quitting belongs to the application, every other key to the scene.
                if (event.key.key == SDLK_ESCAPE)
                {
                    isRunning = false;
                }
                else
                {
                    systems::handle_key(ecs.get_entity_manager(), ids, event.key.key);
                }
            }
        }

        // Elapsed time, used by the systems to animate.
        const Uint64 frameStart = SDL_GetPerformanceCounter();
        seconds = static_cast<float>(
            static_cast<double>(frameStart - start)
            / static_cast<double>(freq));

        // Time since the previous frame, used by the particle simulation.
        // Capped at 0.1 s: after a long freeze (window dragged), the particles must not jump.
        deltaTime = std::min(0.1f, static_cast<float>(
            static_cast<double>(frameStart - previousFrame)
            / static_cast<double>(freq)));
        previousFrame = frameStart;

        // Run the systems: simulate, then draw the frame.
        ecs.execute_systems();

        // FPS in the window title, refreshed once per second.
        const Uint64 now = SDL_GetPerformanceCounter();
        fps_accum += static_cast<double>(now - last) / static_cast<double>(freq);
        fps_count++;
        last = now;

        if (fps_accum >= 1.0)
        {
            const double fps = fps_count / fps_accum;
            const double ms = 1000.0 * fps_accum / fps_count;
            systems::update_window_title(ecs.get_entity_manager(), ids, window, fps, ms);
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
