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
