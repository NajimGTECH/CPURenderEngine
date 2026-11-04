#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "Color3.h"

// A screen position (x to the right, y down, from the top left) with a colour.
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
    void const draw_pixel(SDL_Renderer* renderer, const Pixel& px);
    void const draw_line(SDL_Renderer* renderer, const Pixel& px1, const Pixel& px2);
}