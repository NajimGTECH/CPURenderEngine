#include "Chapters.h"

#include "../Framebuffer.h"

void chapter_1()
{
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            Pixel pixel(x, y, Color3{ static_cast<uint8_t>(x * 255 / WIDTH), 64, static_cast<uint8_t>(y * 255 / HEIGHT) });
            add_pixel_to_buffer(pixel);
        }
    }
}

