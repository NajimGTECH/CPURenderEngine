#include "Tiles.h"

#include "Framebuffer.h"

#include <algorithm>
#include <thread>

// The grid is as square as possible, so the tiles are not long thin strips: 8 threads give 4 columns x 2 rows.
// rows is the largest divisor of count that is not bigger than its square root, so rows * columns is exactly count
// and every thread gets exactly one tile. The screen is wider than tall, so the extra tiles go to the columns.
std::vector<BoundingBox> tiles::make_grid()
{
    // hardware_concurrency returns 0 when it can't tell: then one tile.
    const int count = MULTITHREADING ? static_cast<int>(std::max(1u, std::thread::hardware_concurrency())) : 1;

    int rows = 1;
    for (int divisor = 1; divisor * divisor <= count; divisor++)
    {
        if (count % divisor == 0) rows = divisor;
    }
    const int columns = count / rows;

    std::vector<BoundingBox> grid;
    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            // A tile ends one pixel before the next one starts. When the screen doesn't divide evenly,
            // the integer division spreads the leftover pixels, and the last tile still ends on the screen edge.
            grid.push_back(BoundingBox{
                column * WIDTH / columns,
                row * HEIGHT / rows,
                (column + 1) * WIDTH / columns - 1,
                (row + 1) * HEIGHT / rows - 1
            });
        }
    }

    return grid;
}

void tiles::run_in_parallel(const std::vector<BoundingBox>& grid, const std::function<void(const BoundingBox&)>& draw)
{
    // Without MULTITHREADING: no thread at all, the frame is drawn exactly like before the tiles.
    if (grid.size() == 1)
    {
        draw(grid[0]);
        return;
    }

    // One thread per tile, started every frame. Keeping them alive between frames (a thread pool) would save
    // starting them, but that is small next to filling the pixels (see ARCHITECTURE.md).
    std::vector<std::thread> threads;
    threads.reserve(grid.size());
    for (const BoundingBox& tile : grid)
    {
        threads.emplace_back([&draw, &tile] { draw(tile); });
    }

    // The frame is only finished when the slowest tile is: wait for all of them before going on.
    for (std::thread& thread : threads)
    {
        thread.join();
    }
}
