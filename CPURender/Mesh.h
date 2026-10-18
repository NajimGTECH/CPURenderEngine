#pragma once

#include <string>
#include <vector>

#include "Vec3.h"

// A mesh is just two lists:
//  - vertices: the points in 3D space (the corners)
//  - faces: the triangles, each one names 3 vertices by their index in the list
// Vertices are shared between triangles, that's why faces store indices and not positions.
// Each face corner can also point to a texture coordinate (UV): where that corner lands in the image.

struct UV
{
    float u = 0.0f, v = 0.0f; // both in [0, 1], v = 0 is the BOTTOM of the image
};

struct Face
{
    int a = 0, b = 0, c = 0;          // 0-based indices into Mesh::vertices
    int uvA = -1, uvB = -1, uvC = -1; // 0-based indices into Mesh::uvs, -1 if the corner has none
};

struct Mesh
{
    std::vector<math::Vector3<float>> vertices;
    std::vector<UV> uvs;
    std::vector<Face> faces;
};

namespace mesh
{
    // Minimal Wavefront OBJ loader: reads "v x y z", "vt u v" and "f i j k" lines, ignores the rest.
    // Faces must be triangles. Returns false if the file can't be read or is invalid.
    bool loadObj(const std::string& path, Mesh& outMesh);
}
