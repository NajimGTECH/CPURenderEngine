#include "Pixel.h"

void const px::draw_pixel(SDL_Renderer* renderer, const Pixel& px)
{
    SDL_RenderPoint(renderer, px.x, px.y);
}

void const px::draw_line(SDL_Renderer* renderer, const Pixel& px1, const Pixel& px2)
{
    SDL_RenderLine(renderer, px1.x, px1.y, px2.x, px2.y);
}
