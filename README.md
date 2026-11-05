# CPURenderEngine

A 3D software renderer in C++20: the CPU computes every pixel, and SDL3 only shows the result. It draws textured, lit meshes with several materials, three kinds of light, transparency and particles.

To understand how the code is organised, start with **[ARCHITECTURE.md](ARCHITECTURE.md)**.

## Build and run

Open `CPURender.sln` in Visual Studio, select `x64`, and run. SDL3 is included in `external/`.

## Controls

| Key | Action |
|---|---|
| `1` `2` `3` | Turn the sun, the bulb and the spot on or off |
| `T` | Switch between opaque and transparent meshes |
| `P` | Stop or restart the particle emitters |
| `Esc` | Quit |

## Week 2: reference project analysis

### Multithreading

The window is partitioned into tiles, one per hardware thread (`std::thread::hardware_concurrency()`), and each thread draws the whole scene into its own tile only. Two threads never write the same pixel or depth value, so no lock is needed. Meshes whose box on screen doesn't touch a tile are skipped by it, and particles are drawn by the tile they land in. Every tile runs the three passes in order (opaque, particles, transparent back to front), so the image is exactly the same as on one thread.

Set `MULTITHREADING` to `false` in `CPURender/Tiles.h` to draw on a single thread and compare. Measured in Release on 8 cores, average of 500 frames:

| | One thread | 8 tiles, 8 threads |
|---|---|---|
| Opaque, whole frame | 19.4 ms | 6.7 ms |
| Opaque, `systems::render` only | 18.3 ms | 5.5 ms |
| Transparent (`T`), whole frame | 30.8 ms | 10.5 ms |
| Transparent (`T`), `systems::render` only | 29.4 ms | 9.3 ms |

It is about 3 times faster rather than 8, mostly because the tiles covered by the terrain take much longer than the others. See "Good to know" in [ARCHITECTURE.md](ARCHITECTURE.md).
