# Architecture

A software renderer: the CPU computes every pixel, and SDL3 is only used to open a window and show the result. The scene is organised as an **ECS** (Entity Component System).

## The big picture

```
 main.cpp                the application shell: window, frame loop, time
    |
    |-- Scene.cpp        creates the entities (the only code that knows "the sun", "the cows"...)
    |
    |-- Systems          the behaviour: input, simulation, drawing, on-screen text
    |      |
    |      |-- EntityManager + Components      the data
    |      |
    |      '-- Rasterizer                      triangles and particles -> pixels
    |             |
    |             |-- Shader, Light, Material, Texture     the colour of one pixel
    |             '-- Framebuffer                          where the pixels go
    |
    '-- Chapters/        the course chapters, kept runnable, outside the ECS
```

Each layer only knows the ones below it. The rasterizer has never heard of an entity, and `main.cpp` doesn't know what is in the scene.

## The ECS in three ideas

| Part | What it is | Where |
|---|---|---|
| **Entity** | Just an id (`using Entity = int`). It has no data and no behaviour. | `ComponentStorage.h` |
| **Component** | Plain data: what an entity *is*. | `Components.h` |
| **System** | A function that walks the entities having some components: what they *do*. | `Systems.h` |

So **an entity's behaviour comes only from its components**. Give anything a `SpinComponent` and it spins. No system has a special case for cows.

`EntityManager` (`EntityManager.h`) holds it together. It owns one `ComponentStorage<T>` per component type, which is an array with one slot per entity id, so `get_component<T>(entity)` is an array access. Systems are built on `for_each`:

```cpp
// Every living entity that has both a Transform and a Spin
entities.for_each<TransformComponent, SpinComponent>(
    [&](Entity, TransformComponent& transform, const SpinComponent& spin) {
        transform.rotation = math::Quaternion::FromAxisAngle(spin.axis, seconds * spin.speed);
    });
```

## The entities

All created in `Scene.cpp`. Every entity also has a `NameComponent` and a `TransformComponent`.

| Entity | Mesh | Material | Camera | Light | Particle | Spin | Orbit | LightSweep | LightToggle |
|---|---|---|---|---|---|---|---|---|---|
| camera | | | ✓ | | | | | | |
| sun | | | | ✓ | | | | | key 1 |
| bulb | | | | ✓ | ✓ | | ✓ | | key 2 |
| spot | | | | ✓ | | | | ✓ | key 3 |
| 4 cows | ✓ | ✓ | | | | ✓ | | | |
| fountain | | | | | ✓ | | | | |

The bulb shows the point of this design: it is one entity with a light, a flame and an orbit, so the flame follows the bulb and no code had to link them.

Meshes and materials are **shared**: `MeshComponent` and `MaterialComponent` only hold a pointer, and the four cows point to the same `Mesh`. The data itself lives in `Assets` (`Scene.h`), which must outlive the entities.

## The systems

| System | File | Needs | Does |
|---|---|---|---|
| `handleKey` | `Systems.cpp` | Camera / Particle / Light + LightToggle | Flips transparency (T), emitters (P), lights (their own key) |
| `spinSystem` | `Systems.cpp` | Transform, Spin | Sets the rotation |
| `orbitSystem` | `Systems.cpp` | Transform, Orbit | Moves the entity on its circle |
| `lightSweepSystem` | `Systems.cpp` | Transform, Light, LightSweep | Aims the light |
| `particleSystem` | `Systems.cpp` | Transform, Particle | Spawns, ages and moves the particles |
| `render` | `RenderSystem.cpp` | Transform + Camera, Light, Mesh + Material, Particle | Draws into the framebuffer |
| `drawHud`, `updateWindowTitle` | `HudSystem.cpp` | Name, Light, LightToggle, Particle, Camera | Shows the state of the scene |

Beware of a naming trap: the `ParticleSystem` struct (`Particle.h`) is the pool of living particles, while `particleSystem` above is the ECS system that updates it.

## One frame

```
main loop
 |- SDL events     Esc quits, every other key goes to systems::handleKey
 |- systems::update
 |     spin -> orbit -> light sweep -> particles
 '- update_render
       |- clear the framebuffer
       |- systems::render
       |     find the camera, gather the lights and the meshes, sort them
       |     clear the depth buffer
       |     1. opaque meshes       fill the depth buffer
       |     2. particles           hidden by the opaque meshes
       |     3. transparent meshes  back to front, test the depth but don't write it
       |- upload the framebuffer to an SDL texture
       |- systems::drawHud          text on top, through SDL
       '- present
```

Inside `rasterizer::drawMesh`, for one mesh:

```
vertices -> clip space -> near plane clipping -> divide by w + viewport -> backface culling
         -> for each pixel of the triangle: depth test -> shader::shade -> framebuffer
```

## Where to find things

| File | Role |
|---|---|
| `main.cpp` | Window, frame loop, timing |
| `Scene.h/.cpp` | Asset loading, creation of the entities |
| `Components.h` | Every component |
| `EntityManager.h`, `ComponentStorage.h` | The ECS itself |
| `Systems.h` | The list of systems, and the order they run in |
| `Systems.cpp`, `RenderSystem.cpp`, `HudSystem.cpp` | The systems |
| `Rasterizer.h/.cpp` | Clipping, triangle fill, depth test, transparency, particle drawing |
| `Shader.h/.cpp` | The colour of one pixel (unlit, Lambert, Blinn-Phong) |
| `Light`, `Material`, `Mesh`, `Texture`, `Particle` | The data and the logic of each concept, independent of the ECS |
| `Framebuffer.h/.cpp` | Screen size, frame buffer, depth buffer |
| `Vec3`, `Mat4`, `Quaternion` | The math library |
| `Chapters/` | The course chapters |

## Recipes

### Add an object to the scene

Add a function in `Scene.cpp` and call it from `scene::create`. No system needs to change:

```cpp
// A big cow that turns the other way
static void createBigCow(EntityManager& entities, const Assets& assets)
{
    const Entity entity = entities.create_entity();
    entities.add_component(entity, NameComponent{ "big cow" });

    TransformComponent transform;
    transform.position = math::Vector3<float>(0.0f, 0.0f, -2.0f);
    transform.scale = math::Vector3<float>(2.0f, 2.0f, 2.0f);
    entities.add_component(entity, transform);

    entities.add_component(entity, MeshComponent{ &assets.spot });
    entities.add_component(entity, MaterialComponent{ &assets.spotMaterials[0] });
    entities.add_component(entity, SpinComponent{ { 0.0f, 1.0f, 0.0f }, -1.0f });
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
2. Register it in `registerComponents` (`Scene.cpp`). Using a type that isn't registered throws an exception at runtime.
   ```cpp
   entities.add_component_type<BobComponent>();
   ```
3. Write the system in `Systems.cpp` and call it from `systems::update`:
   ```cpp
   static void bobSystem(EntityManager& entities, float seconds)
   {
       entities.for_each<TransformComponent, BobComponent>(
           [&](Entity, TransformComponent& transform, const BobComponent& bob) {
               transform.position.y = bob.baseHeight + bob.amplitude * std::sin(seconds * bob.speed);
           });
   }
   ```
4. Add a `BobComponent` to any entity in `Scene.cpp`.

### Add a light the user can toggle

Create an entity with a `NameComponent`, a `TransformComponent`, a `LightComponent` and a `LightToggleComponent{ SDLK_4 }`. It works with its key straight away, and shows up on the HUD and in the window title.

### Run an old chapter

In `update_render` (`main.cpp`), uncomment the chapter's call and comment out `systems::render`.

## Conventions

- **Matrices**: row vectors, `p' = p * M`, so `A * B` applies A first. A full transform is `model * view * projection`. The details are at the top of `Mat4.h`.
- **Screen**: the origin is at the top left, y goes down.
- **Colours**: `Vector3` in [0, 1] for math (materials, lights, particles), `Color3` in 8 bits for pixels (textures, framebuffer).
- **Naming**: modules are free functions in a lowercase namespace (`mesh::loadObj`, `systems::render`), components are `XComponent`, and helpers private to a `.cpp` file are `static`.

## Good to know

- **Fixed capacity**: `MAX_ENTITIES` is 100. Every storage reserves a slot per entity, so raising it costs memory for every component type.
- **One camera**: `render` looks through the first entity with a Transform and a Camera. Others are ignored.
- **`Light::position` and `ParticleEmitter::position` are overwritten**: the systems copy them from the entity's Transform, which is the one source of truth. Set the Transform instead.
- **Components are copied when added**, and copying a vector loses its reserved memory. That's why `addParticles` in `Scene.cpp` reserves the particle pool after adding the component.
- **The framebuffer is a global** on purpose: it holds pixels rather than scene data, and the chapters draw into it too.
- **Transparency is approximate**: meshes and faces are sorted from back to front (painter's algorithm), which can fail on faces that intersect.
- **Known bug, chapter 8 draws nothing**: `clear_zbuffer()` in `Chapter08.cpp` is never called, so every pixel fails the depth test. The live scene isn't affected.
