#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "Pixel.h"
#include "Vec3.h"
#include "Mat4.h"
#include "Mesh.h"

#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <limits>
#include <numbers>

// ==============
// SDL Exercices
// ==============

// =============
// Screen

constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 600;
constexpr int CIRCLE_RAY = 200;

// =============
// Buffer

uint32_t frameBuffer[WIDTH * HEIGHT];

void add_pixel_to_buffer(Pixel px)
{
    if (px.x < 0 || px.x >= WIDTH || px.y < 0 || px.y >= HEIGHT) { SDL_Log("Pixel out of screen"); return; }

    frameBuffer[px.y * WIDTH + px.x] = px.color.toRGB();
}

// =============
// Math classes

#pragma region Chap4 maths (deprecated)
struct Vec3
{
    float x, y, z = 1.0f;

    float magnitude()
    {
        return std::sqrt(std::powf(x, 2.0f) + std::powf(y, 2.0f) + std::powf(z, 2.0f));
    }

    Vec3 normalized()
    {
        auto magnitude = this->magnitude();
        if (magnitude == 0.f) return *this;

        return { x / magnitude, y / magnitude, z / magnitude };
    }
};

Vec3 add_vectors(const Vec3& vecA, const Vec3& vecB)
{
    return Vec3{ vecA.x + vecB.x, vecA.y + vecB.y, vecA.z + vecB.z };
}

Vec3 sub_vectors(const Vec3& vecA, const Vec3& vecB)
{
    return Vec3{ vecA.x - vecB.x, vecA.y - vecB.y, vecA.z - vecB.z };
}

Vec3 scale_vectors(float s, const Vec3& vec)
{
    return Vec3{ s * vec.x, s * vec.y, s * vec.z };
}

float dot_product(const Vec3& vecA, const Vec3& vecB)
{
    return vecA.x * vecB.x + vecA.y * vecB.y + vecA.z * vecB.z;
}

Vec3 cross_product(const Vec3& vecA, const Vec3& vecB)
{
    return Vec3{ vecA.y * vecB.z - vecA.z * vecB.y,
        vecA.z * vecB.x - vecA.x * vecB.z,
        vecA.x * vecB.y - vecA.y * vecB.x };
}

struct Mat4
{
    float m[4][4]{};
};
#pragma endregion

// =============
// Chapters

void chapter_1()
{
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            Pixel pixel(x, y, Color3{ static_cast<uint8_t>(x * 255 / WIDTH), 64, static_cast<uint8_t>(y * 255 / WIDTH) });
            add_pixel_to_buffer(pixel);
        }
    }
}

void chapter_4_draw_circle(const Pixel& center, int ray)
{
    // Draw circle

    int x = 0;
    int y = ray;
    int diameter = 3 - 2 * ray;

    while (x <= y)
    {
        add_pixel_to_buffer(Pixel{ center.x + x, center.y + y, center.color });
        add_pixel_to_buffer(Pixel{ center.x - x, center.y + y, center.color });
        add_pixel_to_buffer(Pixel{ center.x + x, center.y - y, center.color });
        add_pixel_to_buffer(Pixel{ center.x - x, center.y - y, center.color });

        add_pixel_to_buffer(Pixel{ center.x + y, center.y + x, center.color });
        add_pixel_to_buffer(Pixel{ center.x - y, center.y + x, center.color });
        add_pixel_to_buffer(Pixel{ center.x + y, center.y - x, center.color });
        add_pixel_to_buffer(Pixel{ center.x - y, center.y - x, center.color });

        x++;

        if (diameter > 0)
        {
            y--;
            diameter += 4 * (x - y) + 10;
        }
        else
        {
            diameter += 4 * x + 6;
        }
    }
}

// The edge function: the 2D cross product of (B - A) and (P - A).
// Sign = which side of edge A->B the point P is on.
// Magnitude = twice the signed area of triangle (A, B, P).
// All integer math. 
float chapter_3_edge(const Pixel& a, const Pixel& b, const Pixel& p)
{
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

void chapter_3(Pixel& pA, Pixel& pB, Pixel& pC)
{
    float area = chapter_3_edge(pA, pB, pC);
    if (area == 0.f) return;
    if (area < 0.f)
    {
        std::swap(pB.x, pC.x);
        std::swap(pB.y, pC.y);
        std::swap(pB.color, pC.color);
        area = -area;
    }

    const float min_x = std::max(0, std::min({ pA.x, pB.x, pC.x }));
    const float min_y = std::max(0, std::min({ pA.y, pB.y, pC.y }));
    const float max_x = std::min(WIDTH - 1, std::max({ pA.x, pB.x, pC.x }));
    const float max_y = std::min(HEIGHT - 1, std::max({ pA.y, pB.y, pC.y }));

    for (int y = min_y; y < max_y; y++)
    {
        for (int x = min_x; x < max_x; x++)
        {
            Pixel point{ x, y };
            float edgeA = chapter_3_edge(pB, pC, point);
            float edgeB = chapter_3_edge(pC, pA, point);
            float edgeC = chapter_3_edge(pA, pB, point);

            // Inside if all three weights are non negative
            if (edgeA >= 0 and edgeB >= 0 and edgeC >= 0)
            {
                // Calculate normalized barycentric weights
                const float weightA = edgeA / area;
                const float weightB = edgeB / area;
                const float weightC = edgeC / area;

                // Interpolate the RGB channels
                const float red = weightA * pA.color.r + weightB * pB.color.r + weightC * pC.color.r;
                const float green = weightA * pA.color.g + weightB * pB.color.g + weightC * pC.color.g;
                const float blue = weightA * pA.color.b + weightB * pB.color.b + weightC * pC.color.b;

                // Clamp and round the channels to the valid RGB range
                point.color.r = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(red)), 0, 255));
                point.color.g = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(green)), 0, 255));
                point.color.b = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(blue)), 0, 255));

                add_pixel_to_buffer(point);
            }
        }
    }
}

// Rotating RGB triangle inside its reference circle.
void chapter_4(float seconds)
{
    const float angle = seconds;

    // Define the triangle in local coordinates.
    const math::Vector3<float> baseA(
        0.0f, -static_cast<float>(CIRCLE_RAY), 0.0f);

    const float halfBase =
        static_cast<float>(CIRCLE_RAY) * 0.8660254f;

    const math::Vector3<float> baseB(
        halfBase, CIRCLE_RAY * 0.5f, 0.0f);

    const math::Vector3<float> baseC(
        -halfBase, CIRCLE_RAY * 0.5f, 0.0f);

    // Create the Z-axis rotation matrix.
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    const math::Mat4<float> rotation(
        c, s, 0.0f, 0.0f,
        -s, c, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    // Translate the triangle to the center of the screen.
    const math::Mat4<float> translation =
        math::Mat4<float>::Translate(
            math::Vector3<float>(
                WIDTH * 0.5f,
                HEIGHT * 0.5f,
                0.0f
            )
        );

    // Apply rotation first, then translation.
    const math::Mat4<float> transform = rotation * translation;

    // Transform all three vertices.
    const auto transformedA = transform.MultiplyPointAffine(baseA);
    const auto transformedB = transform.MultiplyPointAffine(baseB);
    const auto transformedC = transform.MultiplyPointAffine(baseC);

    // Draw the reference circle.
    chapter_4_draw_circle(
        Pixel{
            static_cast<int>(WIDTH * 0.5f),
            static_cast<int>(HEIGHT * 0.5f),
            Color3{ 60, 60, 80, 255 }
        },
        CIRCLE_RAY
    );

    // Convert transformed vertices to pixels.
    Pixel pA{
        static_cast<int>(transformedA.x), static_cast<int>(transformedA.y),
        Color3{ 255, 0, 0, 255 }
    };

    Pixel pB{
        static_cast<int>(transformedB.x), static_cast<int>(transformedB.y),
        Color3{ 0, 255, 0, 255 }
    };

    Pixel pC{
        static_cast<int>(transformedC.x), static_cast<int>(transformedC.y),
        Color3{ 0, 0, 255, 255 }
    };

    // Rasterize the RGB triangle.
    chapter_3(pA, pB, pC);
}

// Bresenham line: integer-only walk from one pixel to the other.
// "error" tracks how far the ideal line is from the pixel we're on,
// and tells us when to step in x, in y, or in both.
void chapter_5_draw_line(const Pixel& from, const Pixel& to)
{
    int x = from.x;
    int y = from.y;

    const int dx = std::abs(to.x - from.x);
    const int dy = -std::abs(to.y - from.y);
    const int stepX = (from.x < to.x) ? 1 : -1;
    const int stepY = (from.y < to.y) ? 1 : -1;
    int error = dx + dy;

    while (true)
    {
        add_pixel_to_buffer(Pixel{ x, y, from.color });
        if (x == to.x && y == to.y) break;

        const int doubleError = 2 * error;
        if (doubleError >= dy) { error += dy; x += stepX; }
        if (doubleError <= dx) { error += dx; y += stepY; }
    }
}

// Cube: 8 corners, and 12 edges joining two corners by index.
const math::Vector3<float> CHAPTER_5_CUBE_VERTICES[8] =
{
    { -1.0f, -1.0f, -1.0f },
    {  1.0f, -1.0f, -1.0f },
    {  1.0f,  1.0f, -1.0f },
    { -1.0f,  1.0f, -1.0f },

    { -1.0f, -1.0f,  1.0f },
    {  1.0f, -1.0f,  1.0f },
    {  1.0f,  1.0f,  1.0f },
    { -1.0f,  1.0f,  1.0f }
};

struct CubeEdge
{
    int start = 0, end = 0;
    int axis = 0; // 0 = X, 1 = Y, 2 = Z (the axis the edge is parallel to, used for its color)
};

const CubeEdge CHAPTER_5_CUBE_EDGES[12] =
{
    { 0, 1, 0 }, { 3, 2, 0 }, { 7, 6, 0 }, { 4, 5, 0 },
    { 0, 3, 1 }, { 1, 2, 1 }, { 5, 6, 1 }, { 4, 7, 1 },
    { 0, 4, 2 }, { 1, 5, 2 }, { 2, 6, 2 }, { 3, 7, 2 }
};

const Color3 CHAPTER_5_AXIS_COLORS[3] =
{
    Color3{ 230, 80, 80 },
    Color3{ 80, 210, 90 },
    Color3{ 90, 140, 240 }
};

// Rotation around X (row-vector convention).
math::Mat4<float> chapter_5_rotation_x(float angle)
{
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    return math::Mat4<float>(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c, s, 0.0f,
        0.0f, -s, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

// Rotation around Y (row-vector convention).
math::Mat4<float> chapter_5_rotation_y(float angle)
{
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    return math::Mat4<float>(
        c, 0.0f, s, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -s, 0.0f, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

struct ScreenVertex
{
    int x = 0, y = 0;
    bool visible = false; // false if behind the camera
};

// Model space -> clip space (p * MVP) -> NDC (divide by w) -> screen pixels (viewport).
ScreenVertex chapter_5_project_vertex(const math::Vector3<float>& vertex, const math::Mat4<float>& mvp)
{
    // Clip space. z is not needed for a wireframe, only x, y and w.
    const float clipX = vertex.x * mvp(0, 0) + vertex.y * mvp(1, 0) + vertex.z * mvp(2, 0) + mvp(3, 0);
    const float clipY = vertex.x * mvp(0, 1) + vertex.y * mvp(1, 1) + vertex.z * mvp(2, 1) + mvp(3, 1);
    const float clipW = vertex.x * mvp(0, 3) + vertex.y * mvp(1, 3) + vertex.z * mvp(2, 3) + mvp(3, 3);

    // w is the distance in front of the camera: <= 0 means behind it (and we can't divide by 0)
    if (clipW <= 0.0001f) return {};

    // Perspective divide: far points get divided by a bigger w, so they end up closer to the center
    const float ndcX = clipX / clipW;
    const float ndcY = clipY / clipW;

    // Viewport: NDC [-1, 1] -> pixels (Y is flipped: up in NDC is down on screen)
    return {
        static_cast<int>((ndcX + 1.0f) * 0.5f * WIDTH),
        static_cast<int>((1.0f - ndcY) * 0.5f * HEIGHT),
        true
    };
}

// Spinning wireframe cube through the full 3D pipeline: Model -> View -> Projection.
void chapter_5(float seconds)
{
    // Model: rotate the cube around its X and Y axes
    const math::Mat4<float> model = chapter_5_rotation_x(seconds * 0.5f) * chapter_5_rotation_y(seconds * 0.8f);

    // View: camera at (0, 0, 5), looking at the origin, +Y is up
    const math::Mat4<float> view = math::Mat4<float>::LookAt(
        math::Vector3<float>(0.0f, 0.0f, 5.0f),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f)
    );

    // Projection: 60 degrees vertical field of view
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 60.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(fovY, aspect, 0.1f, 100.0f);

    // Row vectors (p * M): model, then view, then projection
    const math::Mat4<float> mvp = model * view * projection;

    // Project the 8 corners once, the edges reuse them by index
    ScreenVertex screenVertices[8];
    for (int i = 0; i < 8; i++)
    {
        screenVertices[i] = chapter_5_project_vertex(CHAPTER_5_CUBE_VERTICES[i], mvp);
    }

    for (const CubeEdge& edge : CHAPTER_5_CUBE_EDGES)
    {
        const ScreenVertex& start = screenVertices[edge.start];
        const ScreenVertex& end = screenVertices[edge.end];
        if (!start.visible || !end.visible) continue;

        const Color3& color = CHAPTER_5_AXIS_COLORS[edge.axis];
        chapter_5_draw_line(Pixel{ start.x, start.y, color }, Pixel{ end.x, end.y, color });
    }
}

// Spinning wireframe mesh loaded from an OBJ file.
// Every vertex goes through the 3D pipeline (model -> view -> projection -> divide -> viewport),
// then each triangle is drawn as 3 lines, shaded by depth (bright = near, dim = far).
void chapter_6(const Mesh& mesh, float seconds)
{
    // Projection: 50 degrees vertical field of view
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(fovY, aspect, 0.1f, 100.0f);

    // View: camera 3 units back, looking at the origin where the mesh sits, +Y is up
    const math::Mat4<float> view = math::Mat4<float>::LookAt(
        math::Vector3<float>(0.0f, 0.0f, 3.0f),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f)
    );

    // Model: spin around the Y axis
    const math::Mat4<float> model = math::Mat4<float>::Rotate(
        math::Quaternion::FromAxisAngle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f)
    );

    // Row vectors (p * M): model is applied first, then view.
    const math::Mat4<float> modelView = model * view;

    // Project every vertex once, faces will reuse them by index.
    struct ProjectedVertex
    {
        int x = 0, y = 0;
        float depth = 0.0f;   // distance in front of the camera (bigger = farther)
        bool visible = false; // false if behind the camera
    };

    std::vector<ProjectedVertex> projected(mesh.vertices.size());
    float minDepth = std::numeric_limits<float>::max();
    float maxDepth = std::numeric_limits<float>::lowest();

    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        // Model space -> camera space
        const math::Vector3<float> cameraPos = modelView.MultiplyPointAffine(mesh.vertices[i]);

        // The camera looks down -Z, so the distance in front of it is -z (this is also clip.w).
        // We shade by this depth instead of NDC z: NDC z barely changes across a small object,
        // every edge would end up the same grey.
        const float depth = -cameraPos.z;
        if (depth <= 0.0001f) continue;

        // Camera space -> NDC [-1, 1] (MultiplyPoint does the perspective divide by w)
        const math::Vector3<float> ndc = projection.MultiplyPoint(cameraPos);

        // NDC -> screen pixels (Y is flipped: up in NDC is down on screen)
        projected[i].x = static_cast<int>((ndc.x + 1.0f) * 0.5f * WIDTH);
        projected[i].y = static_cast<int>((1.0f - ndc.y) * 0.5f * HEIGHT);
        projected[i].depth = depth;
        projected[i].visible = true;

        minDepth = std::min(minDepth, depth);
        maxDepth = std::max(maxDepth, depth);
    }

    const float depthRange = (maxDepth > minDepth) ? (maxDepth - minDepth) : 1.0f;

    for (const Face& face : mesh.faces)
    {
        const ProjectedVertex& vA = projected[face.a];
        const ProjectedVertex& vB = projected[face.b];
        const ProjectedVertex& vC = projected[face.c];
        if (!vA.visible || !vB.visible || !vC.visible) continue;

        // Depth cue: map the mesh's own near..far span to a bright..dim grey,
        // so it stays readable whatever the distance to the camera.
        const float averageDepth = (vA.depth + vB.depth + vC.depth) / 3.0f;
        const float far01 = (averageDepth - minDepth) / depthRange; // 0 = near, 1 = far
        const uint8_t level = static_cast<uint8_t>(std::clamp(240 - static_cast<int>(far01 * 200.0f), 0, 255));
        const Color3 color{ level, level, level };

        const Pixel pA{ vA.x, vA.y, color };
        const Pixel pB{ vB.x, vB.y, color };
        const Pixel pC{ vC.x, vC.y, color };

        chapter_5_draw_line(pA, pB);
        chapter_5_draw_line(pB, pC);
        chapter_5_draw_line(pC, pA);
    }
}


// =============
// Chapter 8: Backface culling + Z-buffer
// =============

// Ces constantes permettent d'activer ou de desactiver les techniques
// afin de comparer leur influence sur le rendu.
constexpr bool USE_ZBUFFER = true;
constexpr bool CULL_BACKFACES = true;

// Tableau qui stocke la profondeur de chaque pixel de l'ecran.
// Une profondeur plus petite correspond a une surface plus proche
// avec la matrice de projection utilisee ici.
float zBuffer[WIDTH * HEIGHT];

// Reinitialise toutes les profondeurs avant de dessiner une nouvelle image.
// La valeur maximale permet a toute premiere surface visible de passer
// le test de profondeur.
void clear_zbuffer()
{
    std::fill_n(
        zBuffer,
        WIDTH * HEIGHT,
        std::numeric_limits<float>::max());
}

// Calcule l'aire orientee entre trois points A, B et P en 2D.
// Le signe indique de quel cote de l'arete AB se trouve le point P.
// Cette fonction sert a determiner si un pixel appartient au triangle
// et a determiner l'orientation d'un triangle.
int chapter_7_edge(
    int ax, int ay,
    int bx, int by,
    int px, int py)
{
    return (bx - ax) * (py - ay)
        - (by - ay) * (px - ax);
}

// Limite une valeur a un intervalle donne.
// Ici, cette fonction permet de garder les coordonnees dans les limites
// du framebuffer pour eviter d'acceder a des pixels hors de l'ecran.
int chapter_7_clampi(int value, int minimum, int maximum)
{
    return std::clamp(value, minimum, maximum);
}

// Remplit un triangle pixel par pixel.
// Pour chaque pixel du triangle, la fonction calcule sa profondeur
// et utilise eventuellement le Z-buffer pour conserver la surface visible.
void chapter_7_draw_triangle(
    int x0, int y0, float z0,
    int x1, int y1, float z1,
    int x2, int y2, float z2,
    float zMin, float zRange)
{
    // Calcule l'aire orientee du triangle a partir de ses trois sommets.
    int area = chapter_7_edge(x0, y0, x1, y1, x2, y2);

    // Si l'aire est nulle, le triangle est plat en 2D :
    // il ne possede pas de surface a remplir.
    if (area == 0)
    {
        return;
    }

    // Normalise l'orientation du triangle pour que son aire soit positive.
    // Les profondeurs doivent suivre les sommets correspondants :
    // on echange donc aussi z1 et z2.
    if (area < 0)
    {
        std::swap(x1, x2);
        std::swap(y1, y2);
        std::swap(z1, z2);
        area = -area;
    }

    // Calcule la boite englobante du triangle.
    // Elle represente la zone rectangulaire dans laquelle le triangle
    // peut contenir des pixels.
    // Les coordonnees sont limitees aux dimensions de l'ecran.
    const int minX = chapter_7_clampi(
        std::min({ x0, x1, x2 }), 0, WIDTH - 1);

    const int minY = chapter_7_clampi(
        std::min({ y0, y1, y2 }), 0, HEIGHT - 1);

    const int maxX = chapter_7_clampi(
        std::max({ x0, x1, x2 }), 0, WIDTH - 1);

    const int maxY = chapter_7_clampi(
        std::max({ y0, y1, y2 }), 0, HEIGHT - 1);

    // Calcule l'inverse de l'aire une seule fois pour eviter
    // de refaire la meme division pour chaque pixel.
    const float inverseArea = 1.0f / static_cast<float>(area);

    // Parcourt tous les pixels de la boite englobante.
    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            // Calcule trois valeurs d'arete pour le pixel courant.
            // Chaque valeur correspond a une arete du triangle.
            const int w0 = chapter_7_edge(
                x1, y1, x2, y2, x, y);

            const int w1 = chapter_7_edge(
                x2, y2, x0, y0, x, y);

            const int w2 = chapter_7_edge(
                x0, y0, x1, y1, x, y);

            // Si une valeur est negative, le pixel est a l'exterieur
            // du triangle. On passe alors directement au pixel suivant.
            if ((w0 | w1 | w2) < 0)
            {
                continue;
            }

            // Transforme les valeurs d'arete en poids barycentriques.
            // Ces poids indiquent la contribution de chaque sommet
            // a la profondeur du pixel courant.
            const float weight0 =
                static_cast<float>(w0) * inverseArea;

            const float weight1 =
                static_cast<float>(w1) * inverseArea;

            const float weight2 =
                static_cast<float>(w2) * inverseArea;

            // Interpole la profondeur du pixel a partir des profondeurs
            // des trois sommets du triangle.
            const float depth =
                weight0 * z0
                + weight1 * z1
                + weight2 * z2;

            // Convertit les coordonnees 2D du pixel en indice du tableau.
            // Le framebuffer et le Z-buffer utilisent le meme indice.
            const int index = y * WIDTH + x;

            // Si le Z-buffer est active, verifie si le pixel courant
            // est plus proche de la camera que la surface deja enregistree.
            if constexpr (USE_ZBUFFER)
            {
                // Si la profondeur est plus grande ou egale a celle
                // deja stockee, cette surface est cachee ou a la meme
                // profondeur : on ne dessine pas ce pixel.
                if (depth >= zBuffer[index])
                {
                    continue;
                }

                // La nouvelle surface est plus proche :
                // on enregistre sa profondeur.
                zBuffer[index] = depth;
            }

            // Normalise la profondeur entre 0 et 1 pour produire
            // un effet de niveaux de gris.
            // Une surface proche sera claire et une surface eloignee sombre.
            const float normalizedDepth =
                std::clamp(
                    (depth - zMin) / zRange,
                    0.0f,
                    1.0f);

            const uint8_t intensity =
                static_cast<uint8_t>(
                    chapter_7_clampi(
                        240 - static_cast<int>(
                            normalizedDepth * 200.0f),
                        0,
                        255));

            // Ecrit la couleur finale du pixel dans le framebuffer.
            frameBuffer[index] =
                Color3{
                    intensity,
                    intensity,
                    intensity,
                    255
            }.toRGB();
        }
    }
}


// Stocke les informations d'un sommet apres sa projection sur l'ecran.
struct Chapter8ProjectedVertex
{
    int x = 0;          // Position horizontale sur l'ecran.
    int y = 0;          // Position verticale sur l'ecran.
    float depth = 0.0f; // Profondeur apres projection.
    bool visible = false; // Indique si le sommet est devant la camera.
};

// Effectue le rendu plein d'un modele 3D charge depuis un fichier OBJ.
// Le Backface Culling elimine les triangles orientes a l'oppose
// de la camera avant leur rasterisation.
//
// drawnTriangles compte les triangles conserves.
// culledTriangles compte les triangles elimines par le Backface Culling.
void chapter_8(
    const Mesh& mesh,
    float seconds,
    int& drawnTriangles,
    int& culledTriangles)
{
    // Calcule le rapport entre la largeur et la hauteur de l'ecran.
    // Il permet de conserver les proportions lors de la projection.
    const float aspect =
        static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);

    // Convertit le champ de vision vertical de 50 degres en radians.
    // Les fonctions mathematiques utilisent des radians.
    const float fovY =
        50.0f * std::numbers::pi_v<float> / 180.0f;

    // Cree la matrice de projection en perspective.
    // 0.1 et 100.0 definissent les plans proche et lointain de la camera.
    const math::Mat4<float> projection =
        math::Mat4<float>::Perspective(
            fovY, aspect, 0.1f, 100.0f);

    // Cree la matrice de vue.
    // La camera est placee en (0, 0, 3), regarde l'origine
    // et utilise l'axe Y comme direction verticale.
    const math::Mat4<float> view =
        math::Mat4<float>::LookAt(
            math::Vector3<float>(0.0f, 0.0f, 3.0f),
            math::Vector3<float>(0.0f, 0.0f, 0.0f),
            math::Vector3<float>(0.0f, 1.0f, 0.0f));

    // Cree une rotation autour de l'axe Y.
    // L'angle augmente avec le temps, ce qui anime le modele.
    const math::Mat4<float> model =
        math::Mat4<float>::Rotate(
            math::Quaternion::FromAxisAngle(
                math::Vector3<float>(0.0f, 1.0f, 0.0f),
                seconds * 0.7f));

    // Combine la transformation du modele et celle de la camera.
    // Avec la convention de vecteurs utilisee ici, le modele
    // est applique avant la transformation de vue.
    const math::Mat4<float> modelView = model * view;

    // Cree un tableau de sommets projetes.
    // Chaque sommet du maillage possede une entree correspondante.
    std::vector<Chapter8ProjectedVertex> projected(
        mesh.vertices.size());

    // Initialise les profondeurs minimale et maximale.
    // Elles serviront a calculer les niveaux de gris.
    float zMin = std::numeric_limits<float>::max();
    float zMax = std::numeric_limits<float>::lowest();

    // Parcourt tous les sommets du modele 3D.
    for (std::size_t i = 0; i < mesh.vertices.size(); ++i)
    {
        // Transforme le sommet dans le repere de la camera.
        const math::Vector3<float> cameraPosition =
            modelView.MultiplyPointAffine(mesh.vertices[i]);

        // La camera regarde dans la direction Z negative.
        // Si -Z est trop petit, le sommet est derriere la camera
        // ou trop proche du plan de la camera pour etre traite ici.
        if (-cameraPosition.z <= 0.0001f)
        {
            continue;
        }

        // Applique la projection en perspective et la division
        // par la coordonnee homogene necessaire a la perspective.
        // Le resultat contient les coordonnees normalisees NDC.
        const math::Vector3<float> ndc =
            projection.MultiplyPoint(cameraPosition);

        // Convertit la coordonnee NDC X, comprise normalement entre -1 et 1,
        // en coordonnee horizontale sur l'ecran.
        projected[i].x =
            static_cast<int>((ndc.x + 1.0f) * 0.5f * WIDTH);

        // Convertit la coordonnee NDC Y en coordonnee verticale.
        // On inverse Y car les coordonnees de l'ecran augmentent vers le bas.
        projected[i].y =
            static_cast<int>((1.0f - ndc.y) * 0.5f * HEIGHT);

        // Conserve la profondeur NDC pour le Z-buffer.
        projected[i].depth = ndc.z;

        // Indique que ce sommet a ete traite et se trouve devant la camera.
        projected[i].visible = true;

        // Met a jour les profondeurs extremes du modele projete.
        zMin = std::min(zMin, ndc.z);
        zMax = std::max(zMax, ndc.z);
    }

    // Calcule l'etendue des profondeurs.
    // Si toutes les profondeurs sont identiques, utilise 1 pour eviter
    // une division par zero lors de la normalisation.
    const float zRange =
        (zMax > zMin) ? (zMax - zMin) : 1.0f;

    // Reinitialise les compteurs pour cette image.
    drawnTriangles = 0;
    culledTriangles = 0;

    // Parcourt toutes les faces triangulaires du maillage.
    for (const Face& face : mesh.faces)
    {
        // Recupere les trois sommets de la face a partir de leurs indices.
        const Chapter8ProjectedVertex& a = projected[face.a];
        const Chapter8ProjectedVertex& b = projected[face.b];
        const Chapter8ProjectedVertex& c = projected[face.c];

        // Ignore le triangle si au moins un de ses sommets n'a pas ete
        // projete parce qu'il se trouvait derriere la camera.
        if (!a.visible || !b.visible || !c.visible)
        {
            continue;
        }

        // Calcule l'aire orientee du triangle projete.
        // Son signe indique l'ordre d'orientation des sommets a l'ecran.
        const int area = chapter_7_edge(
            a.x, a.y,
            b.x, b.y,
            c.x, c.y);

        // Ignore les triangles degeneres ou vus exactement de profil.
        if (area == 0)
        {
            continue;
        }

        // Si le Backface Culling est active, elimine les triangles
        // dont l'orientation correspond a une face arriere.
        // Le signe depend de la convention des coordonnees utilisee.
        if constexpr (CULL_BACKFACES)
        {
            if (area >= 0)
            {
                // Compte le triangle elimine.
                ++culledTriangles;

                // Passe directement a la face suivante.
                continue;
            }
        }

        // Le triangle a passe le test d'orientation :
        // il sera donc transmis a la fonction de rasterisation.
        ++drawnTriangles;

        // Remplit le triangle et utilise le Z-buffer pour que
        // seule la surface la plus proche soit visible pour chaque pixel.
        chapter_7_draw_triangle(
            a.x, a.y, a.depth,
            b.x, b.y, b.depth,
            c.x, c.y, c.depth,
            zMin, zRange);
    }
}
// =============
// Renderer custom functions

void update_render(SDL_Renderer* renderer, SDL_Texture* texture)
{
    //SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Drawing stuff...
    Pixel pA{ WIDTH / 2, HEIGHT / 2 - CIRCLE_RAY, Color3{255, 0, 0} };
    Pixel pB{ WIDTH / 2 - CIRCLE_RAY, HEIGHT / 2 + CIRCLE_RAY, Color3{0, 255, 0} };
    Pixel pC{ WIDTH / 2 + CIRCLE_RAY, HEIGHT / 2 + CIRCLE_RAY, Color3{0, 0, 255} };
    chapter_3(pA, pB, pC);

    chapter_4_draw_circle(Pixel{ static_cast<int>(WIDTH / 2), static_cast<int>(HEIGHT / 2), Color3{255, 255, 255} }, CIRCLE_RAY);

    SDL_UpdateTexture(texture, 0, frameBuffer, WIDTH * sizeof(uint32_t));
    SDL_RenderTexture(renderer, texture, 0, 0);
    SDL_RenderPresent(renderer);
}

// =============
// Main function

int main()
{
    // Load the mesh once, before the loop (chapter 6)
    Mesh spot;
    if (!mesh::loadObj("assets/spot.obj", spot)) return EXIT_FAILURE;
    std::cout << "Loaded spot.obj:\t" << spot.vertices.size() << " vertices, " << spot.faces.size() << " faces\n";

    // Init
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        SDL_Log("Couldn't init SDL\n");
        return EXIT_FAILURE;
    }

    SDL_Window* window = SDL_CreateWindow("Fenetre Test", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
    if (window == nullptr) { SDL_Log("Window could not be created.\n"); return EXIT_FAILURE; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) { SDL_Log("Renderer could not be created.\n"); return EXIT_FAILURE; }
    std::cout << "Initialized renderer:\t" << SDL_GetRendererName(renderer) << '\n';

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (texture == nullptr) { SDL_Log("Texture could not be created.\n"); return EXIT_FAILURE; }

    SDL_Event event;

    // FPS measurement
    const Uint64 freq = SDL_GetPerformanceFrequency();
    const Uint64 start = SDL_GetPerformanceCounter();
    Uint64 last = start;
    double fps_accum = 0.0;
    int    fps_count = 0;
    char   title[220];
    int drawnTriangles = 0;
    int culledTriangles = 0;

    // Loop
    bool isRunning = true;
    while (isRunning)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                isRunning = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.key == SDLK_ESCAPE)
                {
                    isRunning = false;
                }
            }
        }

        // Elapsed time, used by the chapters to animate.
        const float seconds = static_cast<float>(
            static_cast<double>(SDL_GetPerformanceCounter() - start)
            / static_cast<double>(freq));

        // Clear the CPU framebuffer.
        const uint32_t backgroundColor =
            Color3{ 16, 16, 24, 255 }.toRGB();

        std::fill_n(frameBuffer, WIDTH * HEIGHT, backgroundColor);
        clear_zbuffer();

        // Draw the current chapter.
        //chapter_4(seconds);
        //chapter_5(seconds);
        //chapter_6(spot, seconds);
        chapter_8(spot, seconds, drawnTriangles, culledTriangles);

        // Upload the framebuffer and display it.
        SDL_RenderClear(renderer);

        SDL_UpdateTexture(
            texture,
            nullptr,
            frameBuffer,
            WIDTH * sizeof(uint32_t)
        );

        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        // FPS in the window title, refreshed once per second.
        const Uint64 now = SDL_GetPerformanceCounter();
        fps_accum += static_cast<double>(now - last) / static_cast<double>(freq);
        fps_count++;
        last = now;

        if (fps_accum >= 1.0)
        {
            const double fps = fps_count / fps_accum;
            const double ms = 1000.0 * fps_accum / fps_count;
            SDL_snprintf(title, sizeof(title), "Chapter 08 - Backface culling | drew %d, culled %d / %d | %.1f FPS (%.2f ms)", drawnTriangles, culledTriangles, static_cast<int>(spot.faces.size()), fps, ms);
            SDL_SetWindowTitle(window, title);
            fps_accum = 0.0;
            fps_count = 0;
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}