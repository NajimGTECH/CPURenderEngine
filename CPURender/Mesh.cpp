#include "Mesh.h"

#include <SDL3/SDL_log.h>

#include <fstream>
#include <sstream>

// A face corner can be "12", "12/3", "12/3/4" or "12//4" (vertex/texture/normal).
// We read the vertex index and the texture index (0 if there is none). Both are still 1-based here.
static void read_face_corner(const std::string& token, int& vertexIndex, int& uvIndex)
{
    const size_t firstSlash = token.find('/');
    vertexIndex = std::stoi(token.substr(0, firstSlash));
    uvIndex = 0;

    if (firstSlash != std::string::npos)
    {
        const size_t secondSlash = token.find('/', firstSlash + 1);
        const std::string uvPart = token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
        if (!uvPart.empty()) uvIndex = std::stoi(uvPart);
    }
}

// Computes one unit normal per face, in the model's local space.
// The cross product of two edges gives a direction perpendicular to the triangle.
static std::vector<math::Vector3<float>> compute_face_normals(const Mesh& mesh)
{
    std::vector<math::Vector3<float>> normals;
    normals.reserve(mesh.faces.size());

    for (const Face& face : mesh.faces)
    {
        const auto& a = mesh.vertices[face.a];
        const auto& b = mesh.vertices[face.b];
        const auto& c = mesh.vertices[face.c];

        const math::Vector3<float> e1(b.x - a.x, b.y - a.y, b.z - a.z);
        const math::Vector3<float> e2(c.x - a.x, c.y - a.y, c.z - a.z);

        math::Vector3<float> normal(
            e1.y * e2.z - e1.z * e2.y,
            e1.z * e2.x - e1.x * e2.z,
            e1.x * e2.y - e1.y * e2.x);

        const float length = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (length > 0.000001f)
        {
            normal.x /= length;
            normal.y /= length;
            normal.z /= length;
        }
        normals.push_back(normal);
    }
    return normals;
}

// One normal per VERTEX: the sum of the normals of the faces touching it, normalized.
// Two neighbouring triangles then share the same normal on their common corners:
// the interpolated normal no longer jumps from one triangle to the next, and the facets disappear.
static std::vector<math::Vector3<float>> compute_vertex_normals(const Mesh& mesh,
    const std::vector<math::Vector3<float>>& faceNormals)
{
    std::vector<math::Vector3<float>> normals(mesh.vertices.size());

    for (size_t faceIndex = 0; faceIndex < mesh.faces.size(); ++faceIndex)
    {
        const Face& face = mesh.faces[faceIndex];
        normals[face.a] += faceNormals[faceIndex];
        normals[face.b] += faceNormals[faceIndex];
        normals[face.c] += faceNormals[faceIndex];
    }

    for (math::Vector3<float>& normal : normals) normal.normalize();
    return normals;
}

bool mesh::load_obj(const std::string& path, Mesh& outMesh)
{
    std::ifstream file(path);
    if (!file.is_open()) { SDL_Log("Could not open %s", path.c_str()); return false; }

    outMesh.vertices.clear();
    outMesh.uvs.clear();
    outMesh.faces.clear();

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream stream(line);
        std::string type;
        stream >> type;

        if (type == "v")
        {
            // Vertex: "v x y z"
            math::Vector3<float> vertex;
            if (stream >> vertex.x >> vertex.y >> vertex.z)
            {
                outMesh.vertices.push_back(vertex);
            }
        }
        else if (type == "vt")
        {
            // Texture coordinate: "vt u v"
            UV uv;
            if (stream >> uv.u >> uv.v)
            {
                outMesh.uvs.push_back(uv);
            }
        }
        else if (type == "f")
        {
            // Face: "f i j k" or "f i/ti j/tj k/tk"
            std::string tokenA, tokenB, tokenC;
            if (stream >> tokenA >> tokenB >> tokenC)
            {
                Face face;
                read_face_corner(tokenA, face.a, face.uvA);
                read_face_corner(tokenB, face.b, face.uvB);
                read_face_corner(tokenC, face.c, face.uvC);

                // OBJ indices start at 1, C++ arrays start at 0 -> subtract 1
                // (a missing UV index was 0, so it becomes -1)
                face.a--; face.b--; face.c--;
                face.uvA--; face.uvB--; face.uvC--;

                outMesh.faces.push_back(face);
            }
        }
        // Any other line type (vn, #comments, ...) is ignored
    }

    // Make sure every face points to an existing vertex (and UV, or -1), so the renderer can't read out of bounds
    const int vertexCount = static_cast<int>(outMesh.vertices.size());
    const int uvCount = static_cast<int>(outMesh.uvs.size());
    for (const Face& face : outMesh.faces)
    {
        if (face.a < 0 || face.a >= vertexCount
            || face.b < 0 || face.b >= vertexCount
            || face.c < 0 || face.c >= vertexCount)
        {
            SDL_Log("Invalid face index in %s", path.c_str());
            return false;
        }

        if (face.uvA < -1 || face.uvA >= uvCount
            || face.uvB < -1 || face.uvB >= uvCount
            || face.uvC < -1 || face.uvC >= uvCount)
        {
            SDL_Log("Invalid texture coordinate index in %s", path.c_str());
            return false;
        }
    }

    // Only once every index is known to be valid
    outMesh.faceNormals = compute_face_normals(outMesh);
    outMesh.vertexNormals = compute_vertex_normals(outMesh, outMesh.faceNormals);

    return true;
}
