#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "Color3.h"

struct Pixel
{
    Pixel(int _x, int _y, Color3 _color) :
        x(_x), y(_y), color(_color) 
    {};

    Pixel(int _x, int _y) :
        x(_x), y(_y), color({0, 0, 0, 255})
    {};

    int x, y = 0;
    Color3 color;
};

namespace px
{
    void const drawPixel(SDL_Renderer* renderer, const Pixel& px);
    void const drawLine(SDL_Renderer* renderer, const Pixel& px1, const Pixel& px2);
}