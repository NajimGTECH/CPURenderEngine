#pragma once

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
extern uint32_t frameBuffer[WIDTH * HEIGHT];

// Depth of the closest thing drawn so far on each pixel (chapter 9).
extern float depthBuffer[WIDTH * HEIGHT];

// Writes one pixel into frameBuffer. A pixel outside the screen is logged and skipped.
void add_pixel_to_buffer(Pixel px);
