#pragma once

#include <array>
#include <cstdint>

#include "Pixel.h"

// The render target: what the CPU draws into. Everything drawn ends up here, one pixel at a time,
// and main.cpp sends frameBuffer to the screen as a texture at the end of the frame.
// It holds pixels, not scene data, so it stays out of the ECS.

// Screen

constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 600;


// Buffer

// One colour per pixel, row by row from the top left, in ARGB8888 (0xAARRGGBB).
extern std::array<uint32_t, WIDTH * HEIGHT> frameBuffer;

// Depth of the closest thing drawn so far on each pixel (chapter 9).
extern std::array<float, WIDTH * HEIGHT> depthBuffer;

// Writes one pixel into frameBuffer. A pixel outside the screen is logged and skipped.
void add_pixel_to_buffer(Pixel px);

// "Over" blending: lays a colour on top of what is already in the framebuffer at this index.
// result = color * opacity + background * (1 - opacity)
Color3 blend_over_framebuffer(const Color3& color, int index, float opacity);
