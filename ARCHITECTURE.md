# Architecture

A software renderer: the CPU computes every pixel, and SDL3 is only used to open a window and show the result. The scene is organised as an **ECS** (Entity Component System).

## The big picture

```
 main.cpp                the application shell: window, frame loop, time, the list of systems
    |
    |-- Scene.cpp        creates the entities (the only code that knows "the sun", "the cows"...)
    |
    |-- ECS              runs the systems, once per frame
    |
    |-- Systems          the behaviour: input, simulation, drawing, on-screen text
    |      |
    |      |-- EntityManager + Components      the data
    |      |
    |      |-- UI                              canvases, images and text -> pixels and SDL text
    |      |
    |      |-- Tiles                           cuts the screen into tiles, draws them on one thread each
    |      |
    |      '-- Rasterizer                      triangles and particles -> the pixels of one tile
    |             |
    |             |-- Shader, Light, Material, Texture     the colour of one pixel
    |             '-- Framebuffer                          where the pixels go
    |
    |-- Editor           generates content: fills a Mesh and a Texture, using Noise
    |
    '-- Chapters/        the course chapters, kept runnable, outside the ECS
```

Each layer only knows the ones below it. The rasterizer has never heard of an entity. `main.cpp` doesn't know what is in the demo scene: it only creates the test content of the week 4 modules, the terrain and the UI panel.

## The ECS in three ideas

| Part | What it is | Where |
|---|---|---|
| **Entity** | Just an `int`. It has no data and no behaviour. | |
| **Component** | Plain data: what an entity *is*. | `Components.h` |
| **System** | A function that walks the entities having some components: what they *do*. | `Systems.h` |

So **an entity's behaviour comes only from its components**. Give anything a `SpinComponent` and it spins. No system has a special case for cows.

## The ECS classes

| Class | Role | Where |
|---|---|---|
| `IComponentStorage` | The interface that lets storages of different types share one list, through `void*` | `ComponentStorage.h` |
| `ComponentStorage<T>` | All the components of one type: an array with one slot per entity id | `ComponentStorage.h` |
| `EntityManager` | Creates the entities, owns the storages, and keeps a bitset of components per entity | `EntityManager.h` |
| `ECS` | Holds the `EntityManager` and the list of systems, and runs them | `ECS.h` |

A **componentID** is an `int`: the index of the storage in the `EntityManager`, and the bit number in the bitsets. `register_component<T>()` returns it, and `scene::create` keeps them all in a `ComponentIDs` struct (`Components.h`) that the systems receive.

Systems are built on `fetch_all_entities`. The components come back as `void*`, so each system casts them to their type:

```cpp
// Every living entity that has both a Transform and a Spin
for (int entity : em.fetch_all_entities(ids.transform, ids.spin))
{
    TransformComponent& transform = *static_cast<TransformComponent*>(em.get_component(entity, ids.transform));
    const SpinComponent& spin = *static_cast<SpinComponent*>(em.get_component(entity, ids.spin));
    const math::Quaternion rotation = math::Quaternion::from_axis_angle(spin.axis, seconds * spin.speed);
    transform.transformMatrix = math::Mat4<float>::trs(transform.transformMatrix.extract_position(), rotation, transform.transformMatrix.extract_scale());
}
```

An `ECS::System` only receives the `EntityManager&`. Everything else a system needs (the ids, the time, the SDL renderer) is captured by the lambda that `main.cpp` adds to the ECS:

```cpp
ecs.add_system([&](EntityManager& em) { systems::spin_system(em, ids, seconds); });
```

## The entities

Created in `Scene.cpp`. Every entity also has a `NameComponent` and a `TransformComponent`.

| Entity | Mesh | Material | Camera | Light | Particle | Spin | Orbit | LightSweep | LightToggle |
|---|---|---|---|---|---|---|---|---|---|
| camera | | | ✓ | | | | | | |
| sun | | | | ✓ | | | | | key 1 |
| bulb | | | | ✓ | ✓ | | ✓ | | key 2 |
| spot | | | | ✓ | | | | ✓ | key 3 |
| 4 cows | ✓ | ✓ | | | | ✓ | | | |
| fountain | | | | | ✓ | | | | |

`main.cpp` adds the test of the week 4 modules. These only have a `NameComponent`, plus:

| Entity | Components |
|---|---|
| terrain | `TransformComponent`, `MeshComponent`, `MaterialComponent` |
| ui panel | `CanvasComponent` |
| ui title, ui caption | `TextComponent` |
| ui minimap | `ImageComponent` |

The bulb shows the point of this design: it is one entity with a light, a flame and an orbit, so the flame follows the bulb and no code had to link them.

Meshes and materials are **shared**: `MeshComponent` and `MaterialComponent` only hold a pointer, and the four cows point to the same `Mesh`. The data itself lives in `Assets` (`Scene.h`), which must outlive the entities.

## The systems

| System | File | Needs | Does |
|---|---|---|---|
| `handle_key` | `Systems.cpp` | Camera / Particle / Canvas / Light + LightToggle | Flips transparency (T), emitters (P), canvases (U), lights (their own key) |
| `spin_system` | `Systems.cpp` | Transform, Spin | Sets the rotation |
| `orbit_system` | `Systems.cpp` | Transform, Orbit | Moves the entity on its circle |
| `light_sweep_system` | `Systems.cpp` | Transform, Light, LightSweep | Aims the light |
| `particle_system` | `Systems.cpp` | Transform, Particle | Spawns, ages and moves the particles |
| `render` | `RenderSystem.cpp` | Transform + Camera, Light, Mesh + Material, Particle | Draws into the framebuffer, one tile per thread |
| `draw_ui_images` | `UISystem.cpp` | Canvas, Image | Draws the images into the framebuffer |
| `draw_ui_text` | `UISystem.cpp` | Canvas, Text | Draws the texts through SDL |
| `draw_hud`, `update_window_title` | `HudSystem.cpp` | Name, Light, LightToggle, Particle, Camera | Shows the state of the scene |

Clearing the framebuffer, uploading it and presenting are systems too, but they only touch SDL and the framebuffer, so they are lambdas in `main.cpp`. `handle_key` and `update_window_title` are not added to the ECS: they use the same API, but run on an SDL event and once per second, not once per frame.

Beware of a naming trap: the `ParticleSystem` struct (`Particle.h`) is the pool of living particles, while `particle_system` above is the ECS system that updates it.

## One frame

```
main loop
 |- SDL events     Esc quits, every other key goes to systems::handle_key
 |- update seconds and deltaTime
 '- ecs.execute_systems()
       |- spin -> orbit -> light sweep -> particles
       |- clear the framebuffer
       |- systems::render
       |     main thread: read the entities into a RenderList of plain data
       |        find the camera, gather the lights, the meshes (+ their box on screen) and the particles
       |        sort the meshes, clear the depth buffer
       |     one thread per tile, all at the same time, each one only writes the pixels of its tile:
       |        1. opaque meshes       fill the depth buffer         skip the meshes that don't touch the tile
       |        2. particles           hidden by the opaque meshes   keep the ones that land in the tile
       |        3. transparent meshes  back to front, test the depth but don't write it
       |     main thread: wait for every tile
       |- systems::draw_ui_images    UI images, into the framebuffer
       |- upload the framebuffer to an SDL texture
       |- systems::draw_ui_text      UI text on top, through SDL
       |- systems::draw_hud          text on top, through SDL
       '- present
```

Inside `rasterizer::draw_mesh`, for one mesh and one tile:

```
vertices -> clip space -> near plane clipping -> divide by w + viewport -> backface culling
         -> for each pixel of the triangle inside the tile: depth test -> shader::shade -> framebuffer
```

With `MULTITHREADING` set to `false` (`Tiles.h`), there is a single tile covering the whole screen, drawn on the main thread: exactly the frame from before the tiles.

## The terrain

`editor::generate_terrain_mesh` and `editor::generate_terrain_texture` run once, before the loop. After that the terrain is an ordinary mesh with an ordinary material: no system knows it was generated.

```
noise::perlin     smooth random values: random directions on a grid, blended with a fade curve
     |
noise::fractal    5 layers of Perlin noise, each one with finer and weaker details
     |
height_at         the height of the ground at a position on the terrain, in [0, 1]
     |
     |-- mesh     a grid of vertices, each one raised by height_at, 2 triangles per square, then the normals
     '-- texture  a texel per position, coloured by height_at: grass -> rock -> snow, with some fine noise
```

The mesh and the texture read the same `height_at`, which is why the snow is on the peaks. Everything comes from `TerrainSettings`, so the same seed always gives the same terrain.

## The UI

| Element | What it is | Reaches the screen |
|---|---|---|
| `Canvas` | An area of the screen. Its elements are placed relative to it, and cut at its edges | |
| `Image` | A texture stretched over a rectangle, with an opacity | Written into the framebuffer, before the upload |
| `Text` | One line of SDL's debug font, with a colour and a scale | Drawn through SDL, after the upload |

An element belongs to a canvas through `canvasEntity`, the id of the canvas entity. Hiding or moving the canvas affects all its elements, and no system has to know the elements individually.

Because of the two steps, a text is always on top of every image, whatever the order of the entities. Inside each step, canvases are drawn in increasing entity id, and so are their elements.

`main.cpp` calls `SDL_SetRenderLogicalPresentation`, which makes SDL's coordinates framebuffer pixels at any window size. Without it the text would drift away from the images, which are stretched with the framebuffer, as soon as the window is resized.

## Where to find things

| File | Role |
|---|---|
| `main.cpp` | Window, frame loop, timing, the list of systems and the order they run in |
| `Scene.h/.cpp` | Asset loading, registration of the components, creation of the entities |
| `Components.h` | Every component, and their `ComponentIDs` |
| `ECS.h`, `EntityManager.h`, `ComponentStorage.h` | The ECS itself |
| `Systems.h` | The declaration of the systems |
| `Systems.cpp`, `RenderSystem.cpp`, `HudSystem.cpp`, `UISystem.cpp` | The systems |
| `Rasterizer.h/.cpp` | Clipping, triangle fill, depth test, transparency, particle drawing |
| `Tiles.h/.cpp` | The grid of tiles, the threads that draw them, and the `MULTITHREADING` switch |
| `Shader.h/.cpp` | The colour of one pixel (unlit, Lambert, Blinn-Phong) |
| `Light`, `Material`, `Mesh`, `Texture`, `Particle` | The data of each concept and its methods, independent of the ECS |
| `Editor.h/.cpp` | Terrain generation: the mesh and the texture |
| `Noise.h/.cpp` | Perlin and fractal noise |
| `UI.h/.cpp` | Canvas, Image and Text, and how to draw them |
| `Framebuffer.h/.cpp` | Screen size, frame buffer, depth buffer, blending |
| `Vec3`, `Mat4`, `Quaternion` | The math library |
| `Chapters/` | The course chapters |

## Recipes

### Add an object to the scene

Add a function in `Scene.cpp` and call it from `scene::create`. No system needs to change:

```cpp
// A big cow that turns the other way
static void create_big_cow(EntityManager& em, const ComponentIDs& ids, const Assets& assets)
{
    const int entity = em.create_entity();

    NameComponent name{ "big cow" };
    em.add_component(entity, ids.name, &name);

    TransformComponent transform;
    transform.transformMatrix = math::Mat4<float>::trs(math::Vector3<float>(0.0f, 0.0f, -2.0f), math::Quaternion(), math::Vector3<float>(2.0f, 2.0f, 2.0f));
    em.add_component(entity, ids.transform, &transform);

    MeshComponent mesh{ &assets.spot };
    em.add_component(entity, ids.mesh, &mesh);

    MaterialComponent material{ &assets.spotMaterials[0] };
    em.add_component(entity, ids.material, &material);

    SpinComponent spin{ { 0.0f, 1.0f, 0.0f }, -1.0f };
    em.add_component(entity, ids.spin, &spin);
}
```

The order of the calls in `scene::create` sets the entity ids, and so the order lights and particles are processed in.

### Add a new behaviour

1. Add the data to `Components.h`:
   ```cpp
   // Moves the entity up and down around its starting height.
   struct BobComponent
   {
       float baseHeight = 0.0f;
       float amplitude = 0.2f;
       float speed = 2.0f;
   };
   ```
2. Add an `int bob = -1;` to `ComponentIDs` (`Components.h`) and register it in `register_components` (`Scene.cpp`). In a Debug build, using an id that isn't registered fails an assert.
   ```cpp
   ids.bob = em.register_component<BobComponent>();
   ```
3. Write the system in `Systems.cpp` and declare it in `Systems.h`:
   ```cpp
   void systems::bob_system(EntityManager& em, const ComponentIDs& ids, float seconds)
   {
       for (int entity : em.fetch_all_entities(ids.transform, ids.bob))
       {
           TransformComponent& transform = *static_cast<TransformComponent*>(em.get_component(entity, ids.transform));
           const BobComponent& bob = *static_cast<BobComponent*>(em.get_component(entity, ids.bob));
           math::Vector3<float> position = transform.transformMatrix.extract_position();
           position.y = bob.baseHeight + bob.amplitude * std::sin(seconds * bob.speed);
           transform.transformMatrix.set_position(position);
       }
   }
   ```
4. Add it to the ECS in `main.cpp`, where it belongs in the frame:
   ```cpp
   ecs.add_system([&](EntityManager& em) { systems::bob_system(em, ids, seconds); });
   ```
5. Add a `BobComponent` to any entity in `Scene.cpp`.

### Add a light the user can toggle

Create an entity with a `NameComponent`, a `TransformComponent`, a `LightComponent` and a `LightToggleComponent{ SDLK_4 }`. It works with its key straight away, and shows up on the HUD and in the window title.

### Run an old chapter

In the drawing system (`main.cpp`), uncomment the chapter's call and comment out `systems::render`.

## Conventions

- **Matrices**: row vectors, `p' = p * M`, so `A * B` applies A first. A full transform is `model * view * projection`. The details are at the top of `Mat4.h`.
- **Screen**: the origin is at the top left, y goes down.
- **Colours**: `Vector3` in [0, 1] for math (materials, lights, particles), `Color3` in 8 bits for pixels (textures, framebuffer).
- **Methods or free functions**: a function that works on one struct is a method of it (`texture.sample_bilinear(u, v)`, `mesh.load_obj(path)`, `box.intersect(tile)`). The structs stay plain data with public fields: the methods only save passing the struct around.
- **Modules**: the code that doesn't belong to one struct is free functions in a lowercase namespace (`rasterizer::draw_mesh`, `systems::render`, `noise::perlin`). They are not turned into classes, which would have no data of their own and would only be namespaces with another name. That's also why a mesh doesn't draw itself: the code that writes pixels stays in the rasterizer and the UI, away from the data. `material::load_mtl` stays free too, because one file creates several materials.
- **Private to a `.cpp` file**: helper functions are `static`, and structs are in an anonymous namespace (`ShaderVertex`, `Drawable`...). Both mean the same thing: nothing outside the file can see them. Nesting such a struct in a class would put it in a header, where every file including it would see it.
- **Naming**: functions and methods are snake_case, components are `XComponent`.

## Good to know

- **Fixed capacity**: `MAX_ENTITIES` is 100. Every storage reserves a slot per entity, so raising it costs memory for every component type. `MAX_COMPONENTS` is 64, the size of the bitsets.
- **Removing a component doesn't clear its slot**: `remove` only marks the slot as free, so what the component holds (like a particle pool) stays allocated until the slot is reused.
- **One camera**: `render` looks through the first entity with a Transform and a Camera. Others are ignored.
- **`Light::position` and `ParticleEmitter::position` are overwritten**: the systems copy them from the entity's Transform, which is the one source of truth. Set the Transform instead.
- **Components are copied when added**, and copying a vector loses its reserved memory. That's why `add_particles` in `Scene.cpp` reserves the particle pool after adding the component.
- **The framebuffer is a global** on purpose: it holds pixels rather than scene data, and the chapters draw into it too.
- **Transparency is approximate**: meshes and faces are sorted from back to front (painter's algorithm), which can fail on faces that intersect.
- **Generating the terrain doesn't cost any FPS**: it takes about 145 ms, once, before the loop. After that it costs what any mesh costs to draw.
- **The terrain is the slowest thing in the frame**: it covers about half of the screen, and the cost is per pixel, so its size on screen matters much more than its number of triangles. Measured in Release, on one thread, without the particles and the UI: 15 ms for the terrain and 11 ms for the cows. Going from 96 to 64 vertices per side only saves about 1 ms.
- **The per-pixel code is written for speed**: `draw_triangle` updates its edge functions with additions instead of computing them for each pixel, and `Texture::sample_bilinear` avoids `std::floor` and `std::round`, which alone took a frame from 49 ms to 28 ms.
- **Multithreading: the threads share nothing they write.** The screen is cut into one tile per hardware thread (4 x 2 tiles with 8 threads), and every thread draws the whole scene but only writes the pixels of its own tile. No two threads ever write the same pixel or depth value, so there is no lock. Everything else (meshes, materials, textures, lights, particles) is only read while the threads run.
- **The threads never touch the ECS**: `render` reads the entities on the main thread first, into a `RenderList` of plain data, and only then starts the threads. The rasterizer only gets a rectangle of pixels (`region`) to stay inside, it doesn't know about threads either.
- **The image is exactly the same with and without threads**, checked byte for byte, opaque and transparent: inside a tile everything is drawn in the same order, and the edge functions give a pixel the same value whatever box the loop starts from.
- **Measured** in Release on a Ryzen 7 9800X3D (8 cores), average of 500 frames: with `MULTITHREADING` the frame goes from 19.4 ms to 6.7 ms (render alone: 18.3 ms to 5.5 ms), and in transparent mode from 30.8 ms to 10.5 ms (render: 29.4 ms to 9.3 ms). So about 3 times faster, not 8.
- **Why not 8 times faster**: the frame has to wait for the slowest tile. The 4 bottom tiles are covered by the terrain and take 4.2 to 4.9 ms each, the 4 top ones (sky and cows) only 1.0 to 1.6 ms. And every tile that touches a mesh transforms all its vertices and sets up all its triangles again, so the 8 tiles add up to about 23 ms of work instead of 18. Smaller tiles handed out to the threads as they finish would balance better, but are more complex.
- **A mesh is skipped by the tiles it doesn't touch**: `rasterizer::screen_box` projects its vertices to find the rectangle it covers. When a vertex is behind the near plane, it gives up and returns the whole screen, which is always safe.
- **The threads are started every frame** and joined at the end of `render`. Keeping them alive between frames (a thread pool) would save that, but it is small: everything `render` does outside the tiles, starting the threads included, takes about 0.6 ms.
- **Known bug, chapter 8 draws nothing**: `clear_zbuffer()` in `Chapter08.cpp` is never called, so every pixel fails the depth test. The live scene isn't affected.
