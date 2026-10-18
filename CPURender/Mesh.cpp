#include "Mesh.h"

#include <SDL3/SDL_log.h>

#include <fstream>
#include <sstream>

// A face token can be "12", "12/3" or "12/3/4" (vertex/texture/normal).
// We only want the vertex index, so we read up to the first '/'.
static int readFaceIndex(const std::string& token)
{
    return std::stoi(token.substr(0, token.find('/')));
}

bool mesh::loadObj(const std::string& path, Mesh& outMesh)
{
    std::ifstream file(path);
    if (!file.is_open()) { SDL_Log("Could not open %s", path.c_str()); return false; }

    outMesh.vertices.clear();
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
        else if (type == "f")
        {
            // Face: "f i j k"
            std::string tokenA, tokenB, tokenC;
            if (stream >> tokenA >> tokenB >> tokenC)
            {
                // OBJ indices start at 1, C++ arrays start at 0 -> subtract 1
                outMesh.faces.push_back(Face{
                    readFaceIndex(tokenA) - 1,
                    readFaceIndex(tokenB) - 1,
                    readFaceIndex(tokenC) - 1 });
            }
        }
        // Any other line type (vt, vn, #comments, ...) is ignored
    }

    // Make sure every face points to an existing vertex, so the renderer can't read out of bounds
    const int vertexCount = static_cast<int>(outMesh.vertices.size());
    for (const Face& face : outMesh.faces)
    {
        if (face.a < 0 || face.a >= vertexCount
            || face.b < 0 || face.b >= vertexCount
            || face.c < 0 || face.c >= vertexCount)
        {
            SDL_Log("Invalid face index in %s", path.c_str());
            return false;
        }
    }

    return true;
}
