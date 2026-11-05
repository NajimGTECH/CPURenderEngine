#pragma once

#include <functional>
#include <vector>

#include "Rasterizer.h"

// Window partitioning: the screen is cut into a grid of tiles, one per hardware thread.
// Every thread draws the whole scene, but only writes the pixels of its own tile,
// so no two threads ever touch the same pixel or depth value and they need no lock.
//
// This module only knows rectangles and threads. What gets drawn in a tile is decided by the caller (RenderSystem.cpp).

// true: one tile per hardware thread, drawn at the same time.
// false: a single tile covering the whole screen, drawn on the main thread, like before the tiles.
// Flip it to compare the frame time (shown in the window title) with and without threads.
constexpr bool MULTITHREADING = true;

namespace tiles
{
    // Cuts the screen into a grid of tiles: one per hardware thread, or a single one without MULTITHREADING.
    std::vector<BoundingBox> make_grid();

    // Calls draw once per tile of the grid, each one on its own thread, and returns when every tile is done.
    // With a single tile, it simply calls draw on the calling thread.
    void run_in_parallel(const std::vector<BoundingBox>& grid, const std::function<void(const BoundingBox&)>& draw);
}
