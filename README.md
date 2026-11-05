# CPURenderEngine

A 3D software renderer and small game engine written from scratch in **C++20**.
Every pixel is computed on the CPU: no GPU, no graphics API, no shaders. SDL3 is only used to open a window, read the keyboard and show the finished image.

The engine draws textured, lit meshes with several materials, three kinds of light, transparency and particles, splits the screen across all CPU cores, and is organised around an **Entity Component System**. On top of it sit an editor module that generates a procedural terrain and a small UI module.

**Authors:** Najim BAKKALI and Tristan GERMAIN

Built by following two courses:
- [rodrigogq/3d-cpu-engine](https://github.com/rodrigogq/3d-cpu-engine): the 12 chapters of the software rendering pipeline
- [HeartBeat-Games/3D-CPU-Engine-Course](https://github.com/HeartBeat-Games/3D-CPU-Engine-Course): the project brief (reference project analysis, ECS, extra modules)

> For a detailed tour of the code, read **[ARCHITECTURE.md](ARCHITECTURE.md)**.

---

## Build and run

Open `CPURender.sln` in Visual Studio, select `x64` (Release for the best frame rate), and run. SDL3 is included in `external/`, nothing else is needed.

### Controls

| Key | Action |
|---|---|
| `1` `2` `3` | Turn the sun, the bulb and the spot light on or off |
| `T` | Switch between opaque and transparent meshes |
| `P` | Stop or restart the particle emitters |
| `U` | Show or hide the UI |
| `Esc` | Quit |

The HUD and the window title show the state of the lights, the number of particles and the frame time.

---

## Part 1: The rendering pipeline (week 1, chapters 1 to 12)

We followed the 12 chapters of the course, writing our own code in C++ instead of the course's C and Python. Every chapter is kept in its own file in [`CPURender/Chapters/`](CPURender/Chapters) and can still be run on its own, so each step of the pipeline can be shown in isolation.

| Chapter | Topic | What we built |
|---|---|---|
| 1 | Framebuffer | An 800×600 ARGB8888 buffer filled on the CPU, uploaded to an SDL texture every frame |
| 2 | Lines | Bresenham's algorithm, integer-only, in every octant (used for the wireframe of chapter 6) |
| 3 | Triangles | Bounding box + edge functions, barycentric weights to blend the three corner colours |
| 4 | Vectors and matrices | Our own math library: `Vector3`, `Mat4` (translation, rotation, scale, inverse, look-at, projection) and **quaternions** |
| 5 | Projection | Model → view → projection → divide by w → viewport, with a perspective projection matrix |
| 6 | Meshes | A Wavefront `.obj` loader (positions, UVs, faces) and Spot the cow drawn in wireframe |
| 7 | Z-buffer | A depth per pixel, so the closest surface wins whatever the drawing order |
| 8 | Backface culling | Triangles facing away from the camera are skipped, about half of the work saved |
| 9 | Textures | A PNG texture mapped with **perspective-correct** UVs (a flag switches to affine to see the PS1-style warping) |
| 10 | Diffuse lighting | Lambert lighting with an ambient term, one normal per face |
| 11 | Smooth shading + specular | Per-vertex normals interpolated per pixel (Phong), and Blinn-Phong highlights |
| 12 | Clipping + filtering | Near-plane clipping in clip space (a triangle becomes 0, 1 or 2 triangles), and bilinear texture filtering |

The math conventions are the same everywhere and written down at the top of [`Mat4.h`](CPURender/Mat4.h): row-major storage, row vectors (`p' = p * M`), so a full transform reads `model * view * projection`.

### The live renderer

Once the chapters were done, we moved what the engine still needs out of them into a proper rendering module: [`Rasterizer`](CPURender/Rasterizer.h), [`Shader`](CPURender/Shader.h), [`Framebuffer`](CPURender/Framebuffer.h). For one mesh, the pipeline is:

```
vertices -> clip space -> near-plane clipping -> divide by w + viewport -> backface culling
         -> for each pixel of the triangle: depth test -> shader::shade -> framebuffer
```

### Performance

A software renderer is slow by nature, so the per-pixel code is written with care:
- edge functions are updated incrementally with additions instead of being recomputed for every pixel,
- the bilinear sampler avoids `std::floor` and `std::round`, which alone took a frame from 49 ms down to 28 ms,
- the specular power is computed with 5 multiplications instead of `std::pow`.

---

## Part 2: Features from the reference projects (week 2)

The second step was to study two existing engines, [3dzavr](https://github.com/vectozavr/3dzavr) and the school's [cpu-engine](https://github.com/sylvainseccia/cpu-engine), and bring their features into ours with the same capabilities.

### Materials and shaders

- A Wavefront **`.mtl` loader** ([`Material`](CPURender/Material.h)) reading `Ka`, `Kd`, `Ks`, `Ns`, `d`, `illum` and `map_Kd`, with texture paths relative to the `.mtl` file.
- The `illum` value picks the material's **shading model**, which acts as our "shader": unlit (flat colour), Lambert (ambient + diffuse) or Blinn-Phong (ambient + diffuse + specular).
- The rasterizer fills a `Fragment` (world position, normal, UV) for each pixel, and [`shader::shade`](CPURender/Shader.h) turns it into a colour from the material and the lights.
- The demo shows four cows sharing **one mesh** with four materials: unlit, Lambert, Phong and gold.

### Lights

Three kinds of light, as in 3dzavr ([`Light`](CPURender/Light.h)), and any number of them can light the scene at once:
- **Directional**: the sun, the same direction and strength everywhere.
- **Point**: a bulb that shines in every direction and gets weaker with distance.
- **Spot**: a point light limited to a cone, with an inner and an outer angle for a soft edge.

### Rendering passes and transparency

A frame is drawn in three passes:
1. **Opaque meshes**, in any order: they fill the depth buffer.
2. **Particles**, hidden by the opaque meshes.
3. **Transparent meshes**: meshes and their faces are sorted from back to front (painter's algorithm), tested against the depth buffer without writing to it, and blended over the framebuffer (`color * opacity + background * (1 - opacity)`).

Opacity comes from the material's `d` value. `T` switches the cows between opaque and transparent.

### Particle system

A pixel particle system based on the school engine ([`Particle`](CPURender/Particle.h)). Each particle is one point in 3D that lights up a single pixel:
- **Emitters**: spawn rate, spawn radius, direction and spread, plus ranges for speed, lifetime and colour.
- **Physics**: gravity, drag, max speed, and an optional box the particles bounce in.
- **Blend modes**: additive for glowing fire and sparks, which fade with age, and opaque for dust and debris.
- A fixed-size pool, so the number of particles never grows past its limit.

The demo has a fountain and a flame attached to the orbiting bulb.

### Fonts

On-screen text with SDL's debug font, used by the HUD and later by the UI module. It is drawn after the framebuffer is uploaded, so it always appears on top of the 3D scene.

### Window partitioning with multithreading

The screen is cut into a grid of **tiles, one per hardware thread** (`std::thread::hardware_concurrency()`), as square as possible: 8 threads give 4 × 2 tiles ([`Tiles`](CPURender/Tiles.h)). Every thread draws the whole scene, but only writes the pixels of its own tile.

- **No locks**: two tiles never share a pixel or a depth value, and everything else (meshes, materials, textures, lights, particles) is only read while the threads run.
- **The threads never touch the ECS**: the render system first reads the entities on the main thread into a `RenderList` of plain data, then starts the threads. The rasterizer only receives a rectangle of pixels to stay inside, it doesn't know about threads either.
- **Tiles skip what they don't see**: each mesh gets a bounding box on screen, and a tile skips the meshes that don't touch it. Particles are only drawn by the tile they land in.
- **Same image, faster**: every tile runs the three passes in the same order, so the result is identical, byte for byte, to the single-threaded one, transparency included.

Set `MULTITHREADING` to `false` in `Tiles.h` to draw on a single thread and compare. Measured in Release on 8 cores, average of 500 frames:

| | One thread | 8 threads |
|---|---|---|
| Opaque, whole frame | 19.4 ms | 6.7 ms |
| Transparent (`T`), whole frame | 30.8 ms | 10.5 ms |

About **3 times faster** rather than 8: the frame waits for the slowest tile, and the 4 bottom tiles, covered by the terrain, take 3 to 4 times longer than the top ones. Details in [ARCHITECTURE.md](ARCHITECTURE.md).

---

## Part 3: The Entity Component System

In the third step (week 3) we rebuilt the project as a game engine around our own ECS, written from scratch. The rendering code became one module of that engine, and the scene is now only data.

### The three ideas

| Part | What it is |
|---|---|
| **Entity** | Just an `int`. It has no data and no behaviour. |
| **Component** | Plain data that says what an entity *is*: `TransformComponent`, `MeshComponent`, `LightComponent`, `SpinComponent`... ([`Components.h`](CPURender/Components.h)) |
| **System** | A function that walks every entity having a given set of components, and says what they *do* ([`Systems.h`](CPURender/Systems.h)) |

An entity's behaviour comes only from its components: give anything a `SpinComponent` and it spins. No system has a special case for cows.

### How it is built

| Class | Role |
|---|---|
| `ComponentStorage<T>` | **One array per component type**, with one slot per entity id, so components of the same type sit next to each other in memory |
| `IComponentStorage` | The interface that lets storages of different types live in the same list |
| `EntityManager` | Creates and destroys entities (destroyed ids are reused), owns the storages, and keeps a **bitset of components per entity** |
| `ECS` | Holds the `EntityManager` and the ordered list of systems, and runs them once per frame |

`register_component<T>()` returns a component id, which is both the index of the storage and a bit number in the bitsets. Finding the entities a system works on is a bitmask test:

```cpp
// Every living entity that has both a Transform and a Spin
for (int entity : em.fetch_all_entities(ids.transform, ids.spin))
{
    TransformComponent& transform = *static_cast<TransformComponent*>(em.get_component(entity, ids.transform));
    const SpinComponent& spin = *static_cast<SpinComponent*>(em.get_component(entity, ids.spin));
    // ...
}
```

Systems are added to the ECS in `main.cpp`, in the order they run. Whatever else a system needs (the time, the SDL renderer...) is captured by a lambda:

```cpp
ecs.add_system([&](EntityManager& em) { systems::spin_system(em, ids, seconds); });
```

### What runs in a frame

```
spin -> orbit -> light sweep -> particles          (simulation)
clear -> render -> UI images -> upload -> UI text -> HUD -> present   (drawing)
```

- **Simulation systems**: `spin_system`, `orbit_system` (the bulb circles the cows), `light_sweep_system` (the spot light scans the scene), `particle_system`.
- **Render system**: finds the camera, gathers the lights, the meshes and the particles into a `RenderList`, then runs the three passes (opaque, particles, transparent) in every tile at once. The rasterizer itself has never heard of an entity.
- **Input**: `handle_key` reacts to keys through components too: a light can be toggled because it has a `LightToggleComponent` holding its key.

The bulb shows the point of this design: it is a single entity with a light, a particle emitter and an orbit, so its flame follows it without any code linking them. Adding an object to the scene means creating an entity in [`Scene.cpp`](CPURender/Scene.cpp), and no system has to change. Meshes and materials are shared through pointers, so the four cows use one mesh.

---

## Part 4: Extra modules (week 4)

Two modules built on top of the engine, both fully integrated into the ECS.

- **Editor module, procedural terrain** ([`Editor`](CPURender/Editor.h), [`Noise`](CPURender/Noise.h)): our own Perlin noise, layered into fractal noise (octaves, lacunarity, gain). The same height function raises a grid of vertices and colours a texture (grass, rock, snow), so the snow always lands on the peaks. The result is an ordinary mesh with an ordinary material: the render system doesn't know it was generated. The same seed always gives the same terrain.
- **UI module** ([`UI`](CPURender/UI.h)): `Canvas`, `Image` and `Text` as components. Elements are placed relative to their canvas and cut at its edges, and hiding a canvas hides everything in it. The demo shows a panel with a title, a caption and a minimap of the terrain.

---

## Project structure

```
CPURender/
├── main.cpp                 window, frame loop, list of systems
├── Scene.cpp                assets and entity creation
├── ECS.h, EntityManager.h, ComponentStorage.h, Components.h
├── Systems.cpp, RenderSystem.cpp, UISystem.cpp, HudSystem.cpp
├── Rasterizer, Shader, Framebuffer           rendering module
├── Tiles                                      screen tiles drawn on one thread each
├── Mesh, Texture, Material, Light, Particle   rendering data
├── Editor, Noise                              terrain module
├── UI                                         UI module
├── Vec3, Mat4, Quaternion                     math library
├── Chapters/                                  the 12 course chapters
└── assets/                                    Spot the cow (OBJ, MTL, PNG)
```
