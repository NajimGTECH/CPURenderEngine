#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

struct Pixel
{
    float x, y = 0.0f;
};

namespace px
{
    void const drawPixel(SDL_Renderer* renderer, const Pixel& px);
    void const drawLine(SDL_Renderer* renderer, const Pixel& px1, const Pixel& px2);
}