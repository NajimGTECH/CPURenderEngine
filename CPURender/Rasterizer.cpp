#include "Rasterizer.h"

#include "Framebuffer.h"
#include "Shader.h"

#include <algorithm>
#include <cmath>
#include <vector>

// The live pipeline: everything the chapters taught that the engine still runs on.
// It knows nothing about entities: the render system (Systems.cpp) feeds it one mesh at a time.

// A projected vertex, like ShadedVertex, that also carries its world position for the shader.
struct ShaderVertex
{
    int x = 0, y = 0;                   // screen position
    float z = 0.0f;                     // NDC depth, for the depth test
    float invW = 0.0f;                  // 1 / w, for perspective-correct interpolation
    UV uv;                              // texture coordinate of this corner
    math::Vector3<float> normal;        // world space normal
    math::Vector3<float> worldPosition; // world space position (point lights, highlight)
};

// The edge function: the 2D cross product of (B - A) and (P - A).
// Sign = which side of edge A->B the point P is on.
// Magnitude = twice the signed area of triangle (A, B, P).
// All integer math. 
float rasterizer::edge(const Pixel& a, const Pixel& b, const Pixel& p)
{
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

// p * M with p = (x, y, z, 1), keeping w: MultiplyPoint would already divide by w,
// which is exactly what we must avoid until the triangle is clipped.
ClipVertex rasterizer::toClipSpace(const math::Mat4<float>& matrix, const math::Vector3<float>& p)
{
    ClipVertex vertex;
    vertex.x = p.x * matrix(0, 0) + p.y * matrix(1, 0) + p.z * matrix(2, 0) + matrix(3, 0);
    vertex.y = p.x * matrix(0, 1) + p.y * matrix(1, 1) + p.z * matrix(2, 1) + matrix(3, 1);
    vertex.z = p.x * matrix(0, 2) + p.y * matrix(1, 2) + p.z * matrix(2, 2) + matrix(3, 2);
    vertex.w = p.x * matrix(0, 3) + p.y * matrix(1, 3) + p.z * matrix(2, 3) + matrix(3, 3);
    return vertex;
}

// Interpolates ALL the attributes: a * (1 - t) + b * t.
static ClipVertex lerpVertex(const ClipVertex& a, const ClipVertex& b, float t)
{
    ClipVertex result;
    result.x = a.x + t * (b.x - a.x);
    result.y = a.y + t * (b.y - a.y);
    result.z = a.z + t * (b.z - a.z);
    result.w = a.w + t * (b.w - a.w);
    result.uv.u = a.uv.u + t * (b.uv.u - a.uv.u);
    result.uv.v = a.uv.v + t * (b.uv.v - a.uv.v);
    result.normal = a.normal + (b.normal - a.normal) * t;
    result.worldPosition = a.worldPosition + (b.worldPosition - a.worldPosition) * t;
    return result;
}

// Signed distance to the near plane, in clip space units. Positive = in front of the near plane (kept).
// In front of near <=> z / w >= -1 <=> z + w >= 0: the test works without dividing by w.
static float nearDistance(const ClipVertex& vertex)
{
    return vertex.z + vertex.w;
}

// Sutherland-Hodgman clipping of a triangle against the near plane.
// We walk the edges A->B, B->C, C->A and for each one we emit:
//  - both ends in front             -> end B
//  - A in front, B behind (leaving) -> the crossing point
//  - A behind, B in front (entering)-> the crossing point, then B
//  - both behind                    -> nothing
// The resulting polygon has 0, 3 or 4 vertices; a quad is split into 2 triangles (0,1,2) and (0,2,3).
// Writes 0, 3 or 6 vertices to out and returns the number of triangles (0, 1 or 2).
int rasterizer::clipTriangleNear(const ClipVertex& vA, const ClipVertex& vB, const ClipVertex& vC, ClipVertex out[6])
{
    const ClipVertex triangle[3] = { vA, vB, vC };
    ClipVertex polygon[4]; // a triangle cut by ONE plane gives at most 4 vertices
    int count = 0;

    for (int edge = 0; edge < 3; edge++)
    {
        const ClipVertex& a = triangle[edge];
        const ClipVertex& b = triangle[(edge + 1) % 3];
        const float distanceA = nearDistance(a);
        const float distanceB = nearDistance(b);
        const bool aInside = distanceA >= 0.0f;
        const bool bInside = distanceB >= 0.0f;

        // The edge crosses the plane: the crossing is at the fraction t where the distance is zero
        if (aInside != bInside) polygon[count++] = lerpVertex(a, b, distanceA / (distanceA - distanceB));
        if (bInside) polygon[count++] = b;
    }

    if (count < 3) return 0;

    out[0] = polygon[0]; out[1] = polygon[1]; out[2] = polygon[2];
    if (count == 3) return 1;

    out[3] = polygon[0]; out[4] = polygon[2]; out[5] = polygon[3];
    return 2;
}

// "Over" blending: lays a colour on top of what is already in the framebuffer.
// result = color * opacity + background * (1 - opacity)
static Color3 blendOverFramebuffer(const Color3& color, int index, float opacity)
{
    // frameBuffer uses ARGB8888 (0xAARRGGBB).
    const uint32_t background = frameBuffer[index];
    const float a = std::clamp(opacity, 0.0f, 1.0f);
    const uint8_t backgroundR = static_cast<uint8_t>((background >> 16) & 0xFF);
    const uint8_t backgroundG = static_cast<uint8_t>((background >> 8) & 0xFF);
    const uint8_t backgroundB = static_cast<uint8_t>(background & 0xFF);

    return Color3{
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(color.r * a + backgroundR * (1.0f - a))), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(color.g * a + backgroundG * (1.0f - a))), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(color.b * a + backgroundB * (1.0f - a))), 0, 255))
    };
}

// Top-left fill rule: a pixel lying exactly on an edge shared by two triangles must be drawn by only ONE of them.
// Opaque triangles don't mind (the second one fails the depth test), but a transparent pixel
// blended twice shows up as a bright line along every edge of the mesh.
// The rule keeps the pixels on the top and left edges of a triangle and drops the others.
// With the winding left after the swap (clockwise on screen, Y down), a "top" edge is horizontal
// and goes right, a "left" edge goes up.
static bool isTopLeftEdge(const Pixel& from, const Pixel& to)
{
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;
    return dy < 0 || (dy == 0 && dx > 0);
}

// Same fill as chapter 11, but the rasterizer no longer knows anything about lighting:
// it builds one Fragment per pixel and the shader decides its colour.
// opacity < 1 blends the pixel over the framebuffer instead of replacing it.
static void drawTriangle(ShaderVertex vA, ShaderVertex vB, ShaderVertex vC, const Material& material,
    const std::vector<Light>& lights, const math::Vector3<float>& cameraPosition, float opacity)
{
    float area = rasterizer::edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y });
    if (area == 0.f) return;
    if (area < 0.f)
    {
        // Swap the whole vertex, so all its attributes follow its position
        std::swap(vB, vC);
        area = -area;
    }

    const Pixel pA{ vA.x, vA.y };
    const Pixel pB{ vB.x, vB.y };
    const Pixel pC{ vC.x, vC.y };

    const int min_x = std::max(0, std::min({ pA.x, pB.x, pC.x }));
    const int min_y = std::max(0, std::min({ pA.y, pB.y, pC.y }));
    const int max_x = std::min(WIDTH - 1, std::max({ pA.x, pB.x, pC.x }));
    const int max_y = std::min(HEIGHT - 1, std::max({ pA.y, pB.y, pC.y }));

    const bool transparent = opacity < 1.0f;

    // Each edge value below is named after the vertex facing that edge (edgeA is edge B->C)
    const bool topLeftA = isTopLeftEdge(pB, pC);
    const bool topLeftB = isTopLeftEdge(pC, pA);
    const bool topLeftC = isTopLeftEdge(pA, pB);

    // Like in chapter 9: every attribute interpolated under perspective is pre-divided by w,
    // the world position included.
    const float uOverWA = vA.uv.u * vA.invW, vOverWA = vA.uv.v * vA.invW;
    const float uOverWB = vB.uv.u * vB.invW, vOverWB = vB.uv.v * vB.invW;
    const float uOverWC = vC.uv.u * vC.invW, vOverWC = vC.uv.v * vC.invW;
    const math::Vector3<float> nOverWA = vA.normal * vA.invW;
    const math::Vector3<float> nOverWB = vB.normal * vB.invW;
    const math::Vector3<float> nOverWC = vC.normal * vC.invW;
    const math::Vector3<float> pOverWA = vA.worldPosition * vA.invW;
    const math::Vector3<float> pOverWB = vB.worldPosition * vB.invW;
    const math::Vector3<float> pOverWC = vC.worldPosition * vC.invW;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            const Pixel point{ x, y };
            const float edgeA = rasterizer::edge(pB, pC, point);
            const float edgeB = rasterizer::edge(pC, pA, point);
            const float edgeC = rasterizer::edge(pA, pB, point);

            // Outside the triangle, or on an edge that belongs to the neighbouring triangle
            if (edgeA < 0 || edgeB < 0 || edgeC < 0) continue;
            if ((edgeA == 0 && !topLeftA) || (edgeB == 0 && !topLeftB) || (edgeC == 0 && !topLeftC)) continue;

            const float weightA = edgeA / area;
            const float weightB = edgeB / area;
            const float weightC = edgeC / area;

            // Depth test
            const float z = weightA * vA.z + weightB * vB.z + weightC * vC.z;
            const int index = y * WIDTH + x;
            if (z >= depthBuffer[index]) continue;

            // Blend the pre-divided attributes, then divide by the blended 1/w to get the real ones back
            const float invW = weightA * vA.invW + weightB * vB.invW + weightC * vC.invW;

            Fragment fragment;
            fragment.uv.u = (weightA * uOverWA + weightB * uOverWB + weightC * uOverWC) / invW;
            fragment.uv.v = (weightA * vOverWA + weightB * vOverWB + weightC * vOverWC) / invW;
            fragment.position = (pOverWA * weightA + pOverWB * weightB + pOverWC * weightC) / invW;
            fragment.normal = (nOverWA * weightA + nOverWB * weightB + nOverWC * weightC) / invW;
            fragment.normal.Normalize(); // see chapter 11

            Color3 color = shader::shade(fragment, material, lights, cameraPosition);

            if (transparent)
            {
                // Transparent fragments only TEST the depth, they don't write it:
                // a see-through surface must not hide what gets drawn after it.
                color = blendOverFramebuffer(color, index, opacity);
            }
            else
            {
                depthBuffer[index] = z;
            }
            add_pixel_to_buffer(Pixel{ x, y, color });
        }
    }
}

// An already clipped triangle (against the near plane, chapter 12):
// divide by w, viewport, backface culling, then the shader rasterizer.
static void projectAndDraw(const ClipVertex triangle[3], const Material& material,
    const std::vector<Light>& lights, const math::Vector3<float>& cameraPosition, float opacity)
{
    ShaderVertex projected[3];
    for (int k = 0; k < 3; k++)
    {
        const float invW = 1.0f / triangle[k].w;
        projected[k].x = static_cast<int>((triangle[k].x * invW + 1.0f) * 0.5f * WIDTH);
        projected[k].y = static_cast<int>((1.0f - triangle[k].y * invW) * 0.5f * HEIGHT);
        projected[k].z = triangle[k].z * invW;
        projected[k].invW = invW;
        projected[k].uv = triangle[k].uv;
        projected[k].normal = triangle[k].normal;
        projected[k].worldPosition = triangle[k].worldPosition;
    }

    // Backface culling (see chapter 9)
    if (rasterizer::edge(Pixel{ projected[0].x, projected[0].y }, Pixel{ projected[1].x, projected[1].y },
        Pixel{ projected[2].x, projected[2].y }) >= 0.0f) return;

    drawTriangle(projected[0], projected[1], projected[2], material, lights, cameraPosition, opacity);
}

// Draws a mesh placed in the world by its model matrix, with a material.
// Same pipeline as chapter 12 (clip space -> near clipping -> raster),
// the vertices also keep their world position for the shader.
// opacity < 1 draws the mesh in the transparent pass: call it AFTER every opaque mesh.
void rasterizer::drawMesh(const Mesh& mesh,
    const math::Mat4<float>& model, const math::Mat4<float>& viewProjection, const Material& material,
    const std::vector<Light>& lights, const math::Vector3<float>& cameraPosition, float opacity)
{
    // Row vectors (p * M): model, then view, then projection.
    const math::Mat4<float> modelViewProjection = model * viewProjection;

    std::vector<ClipVertex> clipVertices(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        clipVertices[i] = toClipSpace(modelViewProjection, mesh.vertices[i]);
        clipVertices[i].normal = model.MultiplyVector(mesh.vertexNormals[i]);
        clipVertices[i].worldPosition = model.MultiplyPointAffine(mesh.vertices[i]);
    }

    // An opaque mesh can draw its faces in any order: the depth buffer sorts them out.
    // A transparent one blends with what is already on screen, so its faces must go
    // from back to front (painter's algorithm). They are sorted by the average clip space w
    // of their corners (= distance in front of the camera): an approximation, but good enough here.
    std::vector<size_t> faceOrder(mesh.faces.size());
    for (size_t i = 0; i < faceOrder.size(); ++i) faceOrder[i] = i;
    if (opacity < 1.0f)
    {
        auto averageW = [&](size_t i) {
            const Face& f = mesh.faces[i];
            return (clipVertices[f.a].w + clipVertices[f.b].w + clipVertices[f.c].w) / 3.0f;
            };
        std::stable_sort(faceOrder.begin(), faceOrder.end(), [&](size_t a, size_t b) {
            return averageW(a) > averageW(b); // farther first
            });
    }

    for (const size_t faceIndex : faceOrder)
    {
        const Face& face = mesh.faces[faceIndex];
        ClipVertex vA = clipVertices[face.a];
        ClipVertex vB = clipVertices[face.b];
        ClipVertex vC = clipVertices[face.c];

        vA.uv = (face.uvA >= 0) ? mesh.uvs[face.uvA] : UV{};
        vB.uv = (face.uvB >= 0) ? mesh.uvs[face.uvB] : UV{};
        vC.uv = (face.uvC >= 0) ? mesh.uvs[face.uvC] : UV{};

        ClipVertex clipped[6];
        const int triangleCount = clipTriangleNear(vA, vB, vC, clipped);
        for (int t = 0; t < triangleCount; t++)
        {
            projectAndDraw(&clipped[t * 3], material, lights, cameraPosition, opacity);
        }
    }
}

// [0, 1] colour -> [0, 255] pixel colour, clamped: added particles quickly go past 1.
static Color3 toColor3(const math::Vector3<float>& color)
{
    return Color3{
        static_cast<uint8_t>(std::clamp(static_cast<int>(color.x * 255.0f), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(color.y * 255.0f), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(color.z * 255.0f), 0, 255))
    };
}

// "Additive" blending: result = background + color * amount.
// Adding light never depends on the order, so unlike the transparent cows the particles need no sorting.
static Color3 blendAdd(const math::Vector3<float>& color, int index, float amount)
{
    // frameBuffer uses ARGB8888 (0xAARRGGBB).
    const uint32_t background = frameBuffer[index];
    const math::Vector3<float> backgroundColor(
        static_cast<float>((background >> 16) & 0xFF) / 255.0f,
        static_cast<float>((background >> 8) & 0xFF) / 255.0f,
        static_cast<float>(background & 0xFF) / 255.0f);

    return toColor3(backgroundColor + color * amount);
}

// Draws every particle of a system as one pixel.
// Same projection as a mesh vertex (chapter 12), then the depth test, so the cows hide the particles behind them.
void rasterizer::drawParticles(const ParticleSystem& system, const math::Mat4<float>& viewProjection)
{
    for (const Particle& particle : system.particles)
    {
        // A particle is a single point: no clipping needed, it is either on screen or not.
        const ClipVertex clip = toClipSpace(viewProjection, particle.position);
        if (clip.w <= 0.0001f) continue; // behind the camera

        const float invW = 1.0f / clip.w;
        const float ndcX = clip.x * invW;
        const float ndcY = clip.y * invW;
        const float z = clip.z * invW;
        if (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f) continue; // outside the window
        if (z < -1.0f || z > 1.0f) continue;                                        // before near or after far

        // ndc = 1 lands exactly on WIDTH or HEIGHT, one pixel past the last one: check the pixel too
        const int x = static_cast<int>((ndcX + 1.0f) * 0.5f * WIDTH);
        const int y = static_cast<int>((1.0f - ndcY) * 0.5f * HEIGHT);
        if (x >= WIDTH || y >= HEIGHT) continue;
        const int index = y * WIDTH + x;
        if (z >= depthBuffer[index]) continue;

        if (particle.blend == ParticleBlend::Intensity)
        {
            // Fades out with age. Squared, so it stays bright most of its life and dies off quickly at the end.
            float fade = particle::lifeLeft(particle);
            fade *= fade;

            // Like the transparent cows: test the depth, don't write it,
            // so the glowing particles behind this one still add their light.
            add_pixel_to_buffer(Pixel{ x, y, blendAdd(particle.color, index, fade) });
        }
        else
        {
            // A solid particle hides what is behind it, like an opaque triangle
            depthBuffer[index] = z;
            add_pixel_to_buffer(Pixel{ x, y, toColor3(particle.color) });
        }
    }
}

