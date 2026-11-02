#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "Framebuffer.h"
#include "EntityManager.h"
#include "Scene.h"
#include "Systems.h"
#include "Chapters/Chapters.h"

#include <stdlib.h>
#include <iostream>
#include <algorithm>

// Keys 1, 2 and 3: turn each light on / off to see what it brings.
static void toggle_light(EntityManager& entities, int entity)
{
    LightComponent& light = entities.get_component<LightComponent>(entity);
    light.enabled = !light.enabled;
}

// Key P: stops / restarts the emitters. The particles already alive finish their life.
static void toggle_particles(EntityManager& entities)
{
    entities.for_each<ParticleComponent>([](int, ParticleComponent& particles) {
        particles.emitting = !particles.emitting;
        });
}

static const char* on_off(bool value)
{
    return value ? "ON" : "OFF";
}

// =============
// Renderer custom functions

// Clears the frame, draws the entities, then sends the framebuffer to the screen.
void update_render(SDL_Renderer* renderer, SDL_Texture* texture, EntityManager& entities, const Scene& scene,
    const RenderSettings& settings, const Assets& assets, float seconds)
{
    // Clear the CPU framebuffer.
    std::fill_n(frameBuffer, WIDTH * HEIGHT, Color3{ 16, 16, 24, 255 }.toRGB());

    // Draw the current chapter. The ECS scene is the default merged result.
    // To go back to an older chapter (Chapters/), uncomment its call and comment out systems::render.
    int drawnTriangles = 0;
    int culledTriangles = 0;
    //chapter_8(assets.spot, seconds, drawnTriangles, culledTriangles);
    //chapter_9(assets.spot, assets.spotTexture, seconds);
    //chapter_10(assets.spot, assets.spotTexture, assets.spot.faceNormals, seconds);
    //chapter_11(assets.spot, assets.spotTexture, assets.spot.vertexNormals, seconds);
    //chapter_12(assets.spot, assets.spotTexture, assets.spot.vertexNormals, seconds);

    // Shaders, materials and particles (keys 1, 2 and 3 for the lights, T for transparency, P for particles)
    systems::render(entities, settings);

    // Upload the CPU framebuffer, then draw readable UI text on top using SDL3's
    // built-in debug font (no additional TTF dependency required).
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(texture, nullptr, frameBuffer, WIDTH * sizeof(uint32_t));
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    const ParticleComponent& fire = entities.get_component<ParticleComponent>(scene.bulb);
    const ParticleComponent& fountain = entities.get_component<ParticleComponent>(scene.fountain);

    char line[128];
    SDL_RenderDebugText(renderer, 12.0f, 12.0f, "3Dzavr CPU Renderer | Shaders + Materials");
    if (settings.transparencyEnabled)
        SDL_snprintf(line, sizeof(line), "Pass: TRANSPARENT | alpha = %.2f | press T to switch", TRANSPARENT_OPACITY);
    else
        SDL_snprintf(line, sizeof(line), "Pass: OPAQUE | press T to switch to transparency");
    SDL_RenderDebugText(renderer, 12.0f, 26.0f, line);
    SDL_snprintf(line, sizeof(line), "Lights: 1 sun %s | 2 bulb %s | 3 spot %s",
        on_off(entities.get_component<LightComponent>(scene.sun).enabled),
        on_off(entities.get_component<LightComponent>(scene.bulb).enabled),
        on_off(entities.get_component<LightComponent>(scene.spot).enabled));
    SDL_RenderDebugText(renderer, 12.0f, 40.0f, line);
    SDL_snprintf(line, sizeof(line), "Particles: P %s | fire %d | fountain %d",
        on_off(fire.emitting),
        static_cast<int>(fire.system.particles.size()), static_cast<int>(fountain.system.particles.size()));
    SDL_RenderDebugText(renderer, 12.0f, 54.0f, line);
    SDL_RenderDebugText(renderer, 12.0f, 68.0f, "ESC: quit");
    SDL_RenderPresent(renderer);
}


// Main function

int main()
{
    // The assets are declared first: the entities point to them.
    Assets assets;
    if (!scene::loadAssets(assets)) return EXIT_FAILURE;

    EntityManager entities;
    const Scene scene = scene::create(entities, assets);
    RenderSettings renderSettings;

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

    SDL_Event event;

    // FPS measurement
    const Uint64 freq = SDL_GetPerformanceFrequency();
    const Uint64 start = SDL_GetPerformanceCounter();
    Uint64 last = start;
    Uint64 previousFrame = start;
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
                else if (event.key.key == SDLK_T)
                {
                    renderSettings.transparencyEnabled = !renderSettings.transparencyEnabled;
                }
                else if (event.key.key == SDLK_1)
                {
                    toggle_light(entities, scene.sun);
                }
                else if (event.key.key == SDLK_2)
                {
                    toggle_light(entities, scene.bulb);
                }
                else if (event.key.key == SDLK_3)
                {
                    toggle_light(entities, scene.spot);
                }
                else if (event.key.key == SDLK_P)
                {
                    toggle_particles(entities);
                }
            }
        }

        // Elapsed time, used by the systems to animate.
        const Uint64 frameStart = SDL_GetPerformanceCounter();
        const float seconds = static_cast<float>(
            static_cast<double>(frameStart - start)
            / static_cast<double>(freq));

        // Time since the previous frame, used by the particle simulation.
        // Capped at 0.1 s: after a long freeze (window dragged), the particles must not jump.
        const float deltaTime = std::min(0.1f, static_cast<float>(
            static_cast<double>(frameStart - previousFrame)
            / static_cast<double>(freq)));
        previousFrame = frameStart;

        // Run the simulation systems, then draw the frame.
        systems::update(entities, seconds, deltaTime);
        update_render(renderer, texture, entities, scene, renderSettings, assets, seconds);

        // FPS in the window title, refreshed once per second.
        const Uint64 now = SDL_GetPerformanceCounter();
        fps_accum += static_cast<double>(now - last) / static_cast<double>(freq);
        fps_count++;
        last = now;

        if (fps_accum >= 1.0)
        {
            const double fps = fps_count / fps_accum;
            const double ms = 1000.0 * fps_accum / fps_count;
            SDL_snprintf(title, sizeof(title), "Shaders - sun %s, bulb %s, spot %s (keys 1/2/3) | %s (key T) | particles %s (key P) | %.1f FPS (%.2f ms)",
                on_off(entities.get_component<LightComponent>(scene.sun).enabled),
                on_off(entities.get_component<LightComponent>(scene.bulb).enabled),
                on_off(entities.get_component<LightComponent>(scene.spot).enabled),
                renderSettings.transparencyEnabled ? "transparent" : "opaque",
                on_off(entities.get_component<ParticleComponent>(scene.bulb).emitting), fps, ms);
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
