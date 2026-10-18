#pragma once

#include <string>
#include <vector>

#include "Vec3.h"

// A mesh is just two lists:
//  - vertices: the points in 3D space (the corners)
//  - faces: the triangles, each one names 3 vertices by their index in the list
// Vertices are shared between triangles, that's why faces store indices and not positions.

struct Face
{
    int a = 0, b = 0, c = 0; // 0-based indices into Mesh::vertices
};

struct Mesh
{
    std::vector<math::Vector3<float>> vertices;
    std::vector<Face> faces;
};

namespace mesh
{
    // Minimal Wavefront OBJ loader: reads "v x y z" and "f i j k" lines, ignores the rest.
    // Faces must be triangles. Returns false if the file can't be read or is invalid.
    bool loadObj(const std::string& path, Mesh& outMesh);
}
