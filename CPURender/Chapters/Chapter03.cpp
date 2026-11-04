#include "Chapters.h"

#include "../Framebuffer.h"
#include "../Rasterizer.h"

#include <algorithm>
#include <cmath>

// Chapter 3: filling a triangle with the edge function, blending its 3 corner colours (barycentric weights).
// The edge function itself now lives in the rasterizer (rasterizer::edge): the live renderer still uses it.

void chapter_3(Pixel& pA, Pixel& pB, Pixel& pC)
{
    float area = rasterizer::edge(pA, pB, pC);
    if (area == 0.f) return;
    if (area < 0.f)
    {
        std::swap(pB.x, pC.x);
        std::swap(pB.y, pC.y);
        std::swap(pB.color, pC.color);
        area = -area;
    }

    const BoundingBox box = rasterizer::bounding_box(pA, pB, pC);

    for (int y = box.minY; y < box.maxY; y++)
    {
        for (int x = box.minX; x < box.maxX; x++)
        {
            Pixel point{ x, y };
            float edgeA = rasterizer::edge(pB, pC, point);
            float edgeB = rasterizer::edge(pC, pA, point);
            float edgeC = rasterizer::edge(pA, pB, point);

            // Inside if all three weights are non negative
            if (edgeA >= 0 and edgeB >= 0 and edgeC >= 0)
            {
                // Calculate normalized barycentric weights
                const float weightA = edgeA / area;
                const float weightB = edgeB / area;
                const float weightC = edgeC / area;

                // Interpolate the RGB channels
                const float red = weightA * pA.color.r + weightB * pB.color.r + weightC * pC.color.r;
                const float green = weightA * pA.color.g + weightB * pB.color.g + weightC * pC.color.g;
                const float blue = weightA * pA.color.b + weightB * pB.color.b + weightC * pC.color.b;

                // Clamp and round the channels to the valid RGB range
                point.color.r = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(red)), 0, 255));
                point.color.g = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(green)), 0, 255));
                point.color.b = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(blue)), 0, 255));

                add_pixel_to_buffer(point);
            }
        }
    }
}

