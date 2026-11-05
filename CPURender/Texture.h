#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Color3.h"

// A texture is a flat 2D image that we wrap onto a mesh.
// Its pixels are called texels (texture elements) to tell them apart from screen pixels.
struct Texture
{
    int width = 0;
    int height = 0;

    // width * height texels; row 0 is the TOP of the image.
    std::vector<Color3> texels;

    // Loads a PNG file (decoded by SDL) into this texture. Returns false if the file can't be read.
    bool load_png(const std::string& path);

    // Nearest-neighbour sampling.
    Color3 sample(float u, float v) const;

    // Bilinear filtering: blends the four nearest texels.
    Color3 sample_bilinear(float u, float v) const;
};
