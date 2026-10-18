#include "Mesh.h"

#include <SDL3/SDL_log.h>

#include <fstream>
#include <sstream>

// A face corner can be "12", "12/3", "12/3/4" or "12//4" (vertex/texture/normal).
// We read the vertex index and the texture index (0 if there is none). Both are still 1-based here.
static void readFaceCorner(const std::string& token, int& vertexIndex, int& uvIndex)
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

bool mesh::loadObj(const std::string& path, Mesh& outMesh)
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
                readFaceCorner(tokenA, face.a, face.uvA);
                readFaceCorner(tokenB, face.b, face.uvB);
                readFaceCorner(tokenC, face.c, face.uvC);

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

    return true;
}
