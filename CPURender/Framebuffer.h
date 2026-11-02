#pragma once

#include <cstdint>

#include "Pixel.h"

// Screen

constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 600;


// Buffer

extern uint32_t frameBuffer[WIDTH * HEIGHT];

// Depth of the closest thing drawn so far on each pixel (chapter 9).
extern float depthBuffer[WIDTH * HEIGHT];

void add_pixel_to_buffer(Pixel px);
