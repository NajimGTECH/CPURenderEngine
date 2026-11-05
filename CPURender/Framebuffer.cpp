#include "Framebuffer.h"

#include <algorithm>
#include <cmath>

std::array<uint32_t, WIDTH * HEIGHT> frameBuffer;

std::array<float, WIDTH * HEIGHT> depthBuffer;

void add_pixel_to_buffer(Pixel px)
{
    if (px.x < 0 || px.x >= WIDTH || px.y < 0 || px.y >= HEIGHT) { SDL_Log("Pixel out of screen"); return; }

    frameBuffer[px.y * WIDTH + px.x] = px.color.to_rgb();
}

Color3 blend_over_framebuffer(const Color3& color, int index, float opacity)
{
    // frameBuffer uses ARGB8888 (0xAARRGGBB).
    const uint32_t background = frameBuffer[index];
    const float a = std::clamp(opacity, 0.0f, 1.0f);
    const uint8_t backgroundR = static_cast<uint8_t>((background >> 16) & 0xFF);
    const uint8_t backgroundG = static_cast<uint8_t>((background >> 8) & 0xFF);
    const uint8_t backgroundB = static_cast<uint8_t>(background & 0xFF);

    return Color3{
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(color.r * a + backgroundR * (1.0f - a))), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(color.g * a + backgroundG * (1.0f - a))), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(color.b * a + backgroundB * (1.0f - a))), 0, 255))
    };
}
