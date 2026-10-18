#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Color3.h"

// A texture is a flat 2D image that we wrap onto a mesh.
// Its pixels are called texels (texture elements) to tell them apart from screen pixels.

struct Texture
{
    int width = 0, height = 0;
    std::vector<Color3> texels; // width * height, row 0 is the TOP of the image
};

namespace texture
{
    // Loads a PNG file (decoded by SDL). Returns false if the file can't be read.
    bool loadPng(const std::string& path, Texture& outTexture);

    // Nearest-neighbour sample: returns the texel closest to the texture coordinate (u, v).
    Color3 sample(const Texture& texture, float u, float v);
}
