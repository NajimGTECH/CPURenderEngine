#pragma once

#include <vector>

#include "../Mesh.h"
#include "../Pixel.h"
#include "../Texture.h"
#include "../Vec3.h"

// The course chapters, one file per chapter. They are kept as they were written, so any of them can
// still be run from main.cpp: they take their mesh directly and don't go through the ECS.
// What the live renderer still needs from them (edge function, clipping) moved to Rasterizer.h.
//
//  1  pixels in the framebuffer
//  3  filling a triangle (edge function)
//  4  first transformations
//  6  3D pipeline, wireframe OBJ mesh
//  8  backface culling + depth buffer (with the chapter 7 helpers in the same file)
//  9  textures, perspective-correct UVs
// 10  diffuse lighting (Lambert), per face
// 11  smooth shading (Phong) + specular highlights, per pixel
// 12  near plane clipping + bilinear filtering

void chapter_1();
void chapter_3(Pixel& pA, Pixel& pB, Pixel& pC);
void chapter_4(float seconds);
void chapter_6(const Mesh& mesh, float seconds);
void chapter_8(const Mesh& mesh, float seconds, int& drawnTriangles, int& culledTriangles);
void chapter_9(const Mesh& mesh, const Texture& meshTexture, float seconds);
void chapter_10(const Mesh& mesh, const Texture& meshTexture,
    const std::vector<math::Vector3<float>>& faceNormals, float seconds);
void chapter_11(const Mesh& mesh, const Texture& meshTexture,
    const std::vector<math::Vector3<float>>& vertexNormals, float seconds);
void chapter_12(const Mesh& mesh, const Texture& meshTexture,
    const std::vector<math::Vector3<float>>& vertexNormals, float seconds);


// Shared between chapters: each one builds on the previous one.

// Flip to false to interpolate the UVs the naive (affine) way:
// the texture warps and swims as the mesh turns, like on the PlayStation 1.
constexpr bool CHAPTER_9_PERSPECTIVE_CORRECT = true;

constexpr float CHAPTER_10_AMBIENT = 0.18f;

// A projected vertex with everything the textured fill needs.
struct TexturedVertex
{
    int x = 0, y = 0;  // screen position
    float z = 0.0f;    // NDC depth, for the depth test
    float invW = 0.0f; // 1 / w, for the perspective-correct UVs
    UV uv;             // texture coordinate of this corner
};

// A projected vertex, like TexturedVertex, that also carries its normal (in world space).
struct ShadedVertex
{
    int x = 0, y = 0;            // screen position
    float z = 0.0f;              // NDC depth, for the depth test
    float invW = 0.0f;           // 1 / w, for perspective-correct interpolation
    UV uv;                       // texture coordinate of this corner
    math::Vector3<float> normal; // vertex normal, already rotated into world space
};

// Used by chapter 12 on top of its clipped triangles.
void chapter_11_draw_triangle_phong(ShadedVertex vA, ShadedVertex vB, ShadedVertex vC, const Texture& meshTexture,
    const math::Vector3<float>& light, const math::Vector3<float>& halfVector, bool bilinear);
