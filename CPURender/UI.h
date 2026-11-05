#pragma once

#include <string>

#include "Color3.h"
#include "Framebuffer.h"
#include "Texture.h"

// The UI module: 2D elements drawn on top of the 3D scene.
// Every position and size is in framebuffer pixels (origin at the top left, y goes down).
// Independent of the ECS, like Light or Particle: UISystem.cpp reads these from the components.

// The area of the screen that holds UI elements.
// An element is placed relative to the top left of its canvas, and whatever goes past the canvas is cut.
// Moving a canvas moves everything on it.
struct Canvas
{
    int x = 0, y = 0;
    int width = WIDTH, height = HEIGHT;
};

// A texture stretched over a rectangle. Width and height must be set.
struct Image
{
    const Texture* texture = nullptr; // not owned: it must outlive the image, like the meshes of the entities
    int x = 0, y = 0;
    int width = 0, height = 0;
    float opacity = 1.0f;             // multiplied by the alpha of each texel
};

// One line of text, using SDL's built-in debug font: 8x8 pixels per character, times scale.
struct Text
{
    std::string content;
    int x = 0, y = 0;
    float scale = 1.0f;
    Color3 color{ 255, 255, 255 };
};

namespace ui
{
    // Writes into the framebuffer, like the rasterizer: call it before the framebuffer is uploaded.
    void draw_image(const Image& image, const Canvas& canvas);

    // Draws through SDL, like the HUD: call it after the framebuffer is uploaded.
    // So a text is always on top of every image.
    void draw_text(const Text& text, const Canvas& canvas, SDL_Renderer* renderer);
}
