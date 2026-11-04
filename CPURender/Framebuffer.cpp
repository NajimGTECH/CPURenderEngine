#include "Framebuffer.h"

std::array<uint32_t, WIDTH * HEIGHT> frameBuffer;

std::array<float, WIDTH * HEIGHT> depthBuffer;

void add_pixel_to_buffer(Pixel px)
{
    if (px.x < 0 || px.x >= WIDTH || px.y < 0 || px.y >= HEIGHT) { SDL_Log("Pixel out of screen"); return; }

    frameBuffer[px.y * WIDTH + px.x] = px.color.to_rgb();
}
