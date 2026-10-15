#include "Pixel.h"

void const px::drawPixel(SDL_Renderer* renderer, const Pixel& px)
{
    SDL_RenderPoint(renderer, px.x, px.y);
}

void const px::drawLine(SDL_Renderer* renderer, const Pixel& px1, const Pixel& px2)
{
    SDL_RenderLine(renderer, px1.x, px1.y, px2.x, px2.y);
}
