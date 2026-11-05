#include "UI.h"

#include "Rasterizer.h"

#include <algorithm>

void ui::draw_image(const Image& image, const Canvas& canvas)
{
    if (image.texture == nullptr || image.width <= 0 || image.height <= 0) return;

    // Where the image is on the screen
    const int left = canvas.x + image.x;
    const int top = canvas.y + image.y;

    // The pixels to walk: the image, cut by its canvas and by the screen
    const BoundingBox box{
        std::max({ left, canvas.x, 0 }),
        std::max({ top, canvas.y, 0 }),
        std::min({ left + image.width, canvas.x + canvas.width, static_cast<int>(WIDTH) }) - 1,
        std::min({ top + image.height, canvas.y + canvas.height, static_cast<int>(HEIGHT) }) - 1
    };

    for (int y = box.minY; y <= box.maxY; y++)
    {
        for (int x = box.minX; x <= box.maxX; x++)
        {
            // + 0.5: the centre of the pixel. v = 0 is the BOTTOM of an image, and y goes down.
            const float u = (static_cast<float>(x - left) + 0.5f) / static_cast<float>(image.width);
            const float v = 1.0f - (static_cast<float>(y - top) + 0.5f) / static_cast<float>(image.height);

            const Color3 texel = texture::sample(*image.texture, u, v);
            const float opacity = image.opacity * static_cast<float>(texel.a) / 255.0f;

            add_pixel_to_buffer(Pixel{ x, y, blend_over_framebuffer(texel, y * WIDTH + x, opacity) });
        }
    }
}

void ui::draw_text(const Text& text, const Canvas& canvas, SDL_Renderer* renderer)
{
    // SDL multiplies every coordinate by the render scale: dividing by it first keeps them in framebuffer pixels.
    SDL_SetRenderScale(renderer, text.scale, text.scale);

    const SDL_Rect clip{
        static_cast<int>(static_cast<float>(canvas.x) / text.scale),
        static_cast<int>(static_cast<float>(canvas.y) / text.scale),
        static_cast<int>(static_cast<float>(canvas.width) / text.scale),
        static_cast<int>(static_cast<float>(canvas.height) / text.scale)
    };
    SDL_SetRenderClipRect(renderer, &clip);

    SDL_SetRenderDrawColor(renderer, text.color.r, text.color.g, text.color.b, text.color.a);
    SDL_RenderDebugText(renderer,
        static_cast<float>(canvas.x + text.x) / text.scale,
        static_cast<float>(canvas.y + text.y) / text.scale,
        text.content.c_str());

    // Back to normal for whatever SDL draws next
    SDL_SetRenderClipRect(renderer, nullptr);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
}
