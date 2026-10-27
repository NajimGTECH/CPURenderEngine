#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include "Pixel.h"
#include "Vec3.h"
#include "Mat4.h"
#include "Mesh.h"
#include "Texture.h"

#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <limits>
#include <numbers>





// Screen

constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 600;
constexpr int CIRCLE_RAY = 200;


// Buffer

uint32_t frameBuffer[WIDTH * HEIGHT];

// Depth of the closest thing drawn so far on each pixel (chapter 9).
float depthBuffer[WIDTH * HEIGHT];

void add_pixel_to_buffer(Pixel px)
{
    if (px.x < 0 || px.x >= WIDTH || px.y < 0 || px.y >= HEIGHT) { SDL_Log("Pixel out of screen"); return; }

    frameBuffer[px.y * WIDTH + px.x] = px.color.toRGB();
}


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


// Chapters

void chapter_1()
{
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            Pixel pixel(x, y, Color3{ static_cast<uint8_t>(x * 255 / WIDTH), 64, static_cast<uint8_t>(y * 255 / HEIGHT) });
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
void chapter_6_draw_line(const Pixel& from, const Pixel& to)
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

        chapter_6_draw_line(pA, pB);
        chapter_6_draw_line(pB, pC);
        chapter_6_draw_line(pC, pA);
    }
}

// Chapter 8: Backface culling + Z-buffer


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


// Flip to false to interpolate the UVs the naive (affine) way:
// the texture warps and swims as the mesh turns, like on the PlayStation 1.
constexpr bool CHAPTER_9_PERSPECTIVE_CORRECT = true;

// A projected vertex with everything the textured fill needs.
struct TexturedVertex
{
    int x = 0, y = 0;  // screen position
    float z = 0.0f;    // NDC depth, for the depth test
    float invW = 0.0f; // 1 / w, for the perspective-correct UVs
    UV uv;             // texture coordinate of this corner
};

// Same barycentric fill as chapter 3, but instead of blending 3 colors we blend 3 texture
// coordinates, then read the color of the texture at that spot.
void chapter_9_draw_triangle_textured(TexturedVertex vA, TexturedVertex vB, TexturedVertex vC, const Texture& meshTexture)
{
    float area = chapter_3_edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y });
    if (area == 0.f) return;
    if (area < 0.f)
    {
        // Swap the whole vertex, so its depth and UV follow its position
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

    // u and v do NOT vary linearly across the screen under perspective, but u/w, v/w and 1/w do.
    // So we pre-divide the UVs by w here, and divide back at each pixel.
    const float uOverWA = vA.uv.u * vA.invW, vOverWA = vA.uv.v * vA.invW;
    const float uOverWB = vB.uv.u * vB.invW, vOverWB = vB.uv.v * vB.invW;
    const float uOverWC = vC.uv.u * vC.invW, vOverWC = vC.uv.v * vC.invW;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            const Pixel point{ x, y };
            const float edgeA = chapter_3_edge(pB, pC, point);
            const float edgeB = chapter_3_edge(pC, pA, point);
            const float edgeC = chapter_3_edge(pA, pB, point);

            // Outside the triangle
            if (edgeA < 0 || edgeB < 0 || edgeC < 0) continue;

            const float weightA = edgeA / area;
            const float weightB = edgeB / area;
            const float weightC = edgeC / area;

            // Depth test: only draw if this pixel is closer than what's already there.
            // NDC z already varies linearly across the screen, so a plain blend is correct.
            const float z = weightA * vA.z + weightB * vB.z + weightC * vC.z;
            const int index = y * WIDTH + x;
            if (z >= depthBuffer[index]) continue;

            float u, v;
            if (CHAPTER_9_PERSPECTIVE_CORRECT)
            {
                // Blend u/w, v/w and 1/w, then divide back to get the true u, v
                const float invW = weightA * vA.invW + weightB * vB.invW + weightC * vC.invW;
                u = (weightA * uOverWA + weightB * uOverWB + weightC * uOverWC) / invW;
                v = (weightA * vOverWA + weightB * vOverWB + weightC * vOverWC) / invW;
            }
            else
            {
                // Naive blend, wrong under perspective
                u = weightA * vA.uv.u + weightB * vB.uv.u + weightC * vC.uv.u;
                v = weightA * vA.uv.v + weightB * vB.uv.v + weightC * vC.uv.v;
            }

            depthBuffer[index] = z;
            add_pixel_to_buffer(Pixel{ x, y, texture::sample(meshTexture, u, v) });
        }
    }
}


void chapter_10_draw_triangle_lit(TexturedVertex vA, TexturedVertex vB, TexturedVertex vC, const Texture& meshTexture, float brightness)
{
    float area = chapter_3_edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y });
    if (area == 0.f) return;
    if (area < 0.f)
    {
        // Swap the whole vertex, so its depth and UV follow its position
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

    // u and v do NOT vary linearly across the screen under perspective, but u/w, v/w and 1/w do.
    // So we pre-divide the UVs by w here, and divide back at each pixel.
    const float uOverWA = vA.uv.u * vA.invW, vOverWA = vA.uv.v * vA.invW;
    const float uOverWB = vB.uv.u * vB.invW, vOverWB = vB.uv.v * vB.invW;
    const float uOverWC = vC.uv.u * vC.invW, vOverWC = vC.uv.v * vC.invW;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            const Pixel point{ x, y };
            const float edgeA = chapter_3_edge(pB, pC, point);
            const float edgeB = chapter_3_edge(pC, pA, point);
            const float edgeC = chapter_3_edge(pA, pB, point);

            // Outside the triangle
            if (edgeA < 0 || edgeB < 0 || edgeC < 0) continue;

            const float weightA = edgeA / area;
            const float weightB = edgeB / area;
            const float weightC = edgeC / area;

            // Depth test: only draw if this pixel is closer than what's already there.
            // NDC z already varies linearly across the screen, so a plain blend is correct.
            const float z = weightA * vA.z + weightB * vB.z + weightC * vC.z;
            const int index = y * WIDTH + x;
            if (z >= depthBuffer[index]) continue;

            float u, v;
            if (CHAPTER_9_PERSPECTIVE_CORRECT)
            {
                // Blend u/w, v/w and 1/w, then divide back to get the true u, v
                const float invW = weightA * vA.invW + weightB * vB.invW + weightC * vC.invW;
                u = (weightA * uOverWA + weightB * uOverWB + weightC * uOverWC) / invW;
                v = (weightA * vOverWA + weightB * vOverWB + weightC * vOverWC) / invW;
            }
            else
            {
                // Naive blend, wrong under perspective
                u = weightA * vA.uv.u + weightB * vB.uv.u + weightC * vC.uv.u;
                v = weightA * vA.uv.v + weightB * vB.uv.v + weightC * vC.uv.v;
            }

            depthBuffer[index] = z;

            // Échantillonne la texture, puis assombrit ses trois canaux RGB.
            Color3 texel = texture::sample(meshTexture, u, v);
            texel.r = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.r * brightness), 0, 255));
            texel.g = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.g * brightness), 0, 255));
            texel.b = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.b * brightness), 0, 255));
            add_pixel_to_buffer(Pixel{ x, y, texel });
        }
    }
}



// Chapitre 10 : éclairage diffus (Lambert)

constexpr float CHAPTER_10_AMBIENT = 0.18f;

// Calcule une normale unitaire par face dans l'espace local du modèle.
// Le produit vectoriel de deux arêtes donne une direction perpendiculaire au triangle.
std::vector<math::Vector3<float>> chapter_10_compute_face_normals(const Mesh& mesh)
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

// Même pipeline que le chapitre 9, avec une luminosité plate calculée pour chaque triangle.
void chapter_10(const Mesh& mesh, const Texture& meshTexture,
    const std::vector<math::Vector3<float>>& faceNormals, float seconds)
{
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(fovY, aspect, 0.1f, 100.0f);

    const math::Mat4<float> view = math::Mat4<float>::LookAt(
        math::Vector3<float>(0.0f, 0.0f, 3.0f),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f));

    const math::Mat4<float> model = math::Mat4<float>::Rotate(
        math::Quaternion::FromAxisAngle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f));
    const math::Mat4<float> modelView = model * view;

    // Direction de la lumière : vecteur normalisé pointant de la surface vers la lumière.
    math::Vector3<float> light(0.4f, 0.8f, 0.45f);
    const float lightLength = std::sqrt(light.x * light.x + light.y * light.y + light.z * light.z);
    light.x /= lightLength;
    light.y /= lightLength;
    light.z /= lightLength;

    std::fill_n(depthBuffer, WIDTH * HEIGHT, std::numeric_limits<float>::max());

    std::vector<TexturedVertex> projected(mesh.vertices.size());
    std::vector<bool> visible(mesh.vertices.size(), false);

    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        const math::Vector3<float> cameraPos = modelView.MultiplyPointAffine(mesh.vertices[i]);
        const float w = -cameraPos.z;
        if (w <= 0.0001f) continue;

        const math::Vector3<float> ndc = projection.MultiplyPoint(cameraPos);
        projected[i].x = static_cast<int>((ndc.x + 1.0f) * 0.5f * WIDTH);
        projected[i].y = static_cast<int>((1.0f - ndc.y) * 0.5f * HEIGHT);
        projected[i].z = ndc.z;
        projected[i].invW = 1.0f / w;
        visible[i] = true;
    }

    for (size_t faceIndex = 0; faceIndex < mesh.faces.size(); ++faceIndex)
    {
        const Face& face = mesh.faces[faceIndex];
        if (!visible[face.a] || !visible[face.b] || !visible[face.c]) continue;

        TexturedVertex vA = projected[face.a];
        TexturedVertex vB = projected[face.b];
        TexturedVertex vC = projected[face.c];

        // Le retournement de l'axe Y à l'écran rend l'aire des faces avant négative.
        // Une aire positive indique une face arrière : on ne la dessine pas.
        if (chapter_3_edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y }) >= 0.0f) continue;

        vA.uv = (face.uvA >= 0) ? mesh.uvs[face.uvA] : UV{};
        vB.uv = (face.uvB >= 0) ? mesh.uvs[face.uvB] : UV{};
        vC.uv = (face.uvC >= 0) ? mesh.uvs[face.uvC] : UV{};

        // La matrice modèle ne fait ici qu'une rotation : elle transforme la normale
        // de l'espace local vers l'espace monde sans changer sa longueur.
        const math::Vector3<float> normal = model.MultiplyPointAffine(faceNormals[faceIndex]);
        float diffuse = normal.x * light.x + normal.y * light.y + normal.z * light.z;
        diffuse = std::max(0.0f, diffuse); // Une face tournée à l'opposé de la lumière ne reçoit pas de lumière diffuse.
        const float brightness = CHAPTER_10_AMBIENT + (1.0f - CHAPTER_10_AMBIENT) * diffuse;

        chapter_10_draw_triangle_lit(vA, vB, vC, meshTexture, brightness);
    }
}

// Spinning textured mesh: filled triangles, a depth buffer so near triangles hide far ones,
// backface culling, and a texture wrapped on it with perspective-correct UVs.
void chapter_9(const Mesh& mesh, const Texture& meshTexture, float seconds)
{
    // Same camera and spin as chapter 6
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(fovY, aspect, 0.1f, 100.0f);

    const math::Mat4<float> view = math::Mat4<float>::LookAt(
        math::Vector3<float>(0.0f, 0.0f, 3.0f),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f)
    );

    const math::Mat4<float> model = math::Mat4<float>::Rotate(
        math::Quaternion::FromAxisAngle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f)
    );

    const math::Mat4<float> modelView = model * view;

    // Reset the depth buffer: nothing drawn yet = infinitely far
    std::fill_n(depthBuffer, WIDTH * HEIGHT, std::numeric_limits<float>::max());

    // Project every vertex once, faces will reuse them by index.
    std::vector<TexturedVertex> projected(mesh.vertices.size());
    std::vector<bool> visible(mesh.vertices.size(), false);

    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        // Model space -> camera space
        const math::Vector3<float> cameraPos = modelView.MultiplyPointAffine(mesh.vertices[i]);

        // The camera looks down -Z, so w (the distance in front of it) is -z
        const float w = -cameraPos.z;
        if (w <= 0.0001f) continue;

        // Camera space -> NDC [-1, 1] (MultiplyPoint does the perspective divide by w)
        const math::Vector3<float> ndc = projection.MultiplyPoint(cameraPos);

        // NDC -> screen pixels (Y is flipped: up in NDC is down on screen)
        projected[i].x = static_cast<int>((ndc.x + 1.0f) * 0.5f * WIDTH);
        projected[i].y = static_cast<int>((1.0f - ndc.y) * 0.5f * HEIGHT);
        projected[i].z = ndc.z;
        projected[i].invW = 1.0f / w;
        visible[i] = true;
    }

    for (const Face& face : mesh.faces)
    {
        if (!visible[face.a] || !visible[face.b] || !visible[face.c]) continue;

        TexturedVertex vA = projected[face.a];
        TexturedVertex vB = projected[face.b];
        TexturedVertex vC = projected[face.c];

        // Backface culling: OBJ front faces are counter-clockwise, and the Y flip of the viewport
        // makes them clockwise on screen, so their area is negative.
        // Positive area = we're looking at the back of the triangle, it's hidden anyway: skip it.
        if (chapter_3_edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y }) >= 0.f) continue;

        // The UV belongs to the face corner, not the vertex: the same vertex can have
        // different UVs on two faces (on a seam of the texture)
        vA.uv = (face.uvA >= 0) ? mesh.uvs[face.uvA] : UV{};
        vB.uv = (face.uvB >= 0) ? mesh.uvs[face.uvB] : UV{};
        vC.uv = (face.uvC >= 0) ? mesh.uvs[face.uvC] : UV{};

        chapter_9_draw_triangle_textured(vA, vB, vC, meshTexture);
    }
}


// Chapitre 11 : smooth shading (Phong) + reflets spéculaires

constexpr float CHAPTER_11_SPEC_STRENGTH = 0.55f; // 0 = pas de reflet, 1 = aussi brillant que la lumière

// Un sommet projeté, comme TexturedVertex, qui porte en plus sa normale (en espace monde).
struct ShadedVertex
{
    int x = 0, y = 0;            // position à l'écran
    float z = 0.0f;              // profondeur NDC, pour le test de profondeur
    float invW = 0.0f;           // 1 / w, pour l'interpolation perspective-correcte
    UV uv;                       // coordonnée de texture de ce coin
    math::Vector3<float> normal; // normale du sommet, déjà tournée en espace monde
};

// Une normale par SOMMET : la somme des normales des faces qui le touchent, normalisée.
// Deux triangles voisins partagent ainsi la même normale sur leurs coins communs :
// la normale interpolée ne saute plus d'un triangle à l'autre, les facettes disparaissent.
std::vector<math::Vector3<float>> chapter_11_compute_vertex_normals(const Mesh& mesh,
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

    for (math::Vector3<float>& normal : normals) normal.Normalize();
    return normals;
}

// x^32 en 5 multiplications (x^2, x^4, x^8, x^16, x^32), bien moins cher que std::pow dans la boucle par pixel.
// La brillance (shininess) est donc fixée à 32.
float chapter_11_pow32(float x)
{
    const float x2 = x * x;
    const float x4 = x2 * x2;
    const float x8 = x4 * x4;
    const float x16 = x8 * x8;
    return x16 * x16;
}

// Même remplissage que le chapitre 9, mais l'éclairage est calculé à chaque PIXEL (Phong) :
// la normale est interpolée (perspective-correcte, comme les UV), re-normalisée,
// puis on évalue le diffus de Lambert et le spéculaire de Blinn-Phong.
// bilinear choisit l'échantillonnage de la texture (chapitre 12).
void chapter_11_draw_triangle_phong(ShadedVertex vA, ShadedVertex vB, ShadedVertex vC, const Texture& meshTexture,
    const math::Vector3<float>& light, const math::Vector3<float>& halfVector, bool bilinear,
    bool transparentPass = false, float opacity = 1.0f)
{
    float area = chapter_3_edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y });
    if (area == 0.f) return;
    if (area < 0.f)
    {
        // On échange le sommet entier, pour que sa profondeur, son UV et sa normale suivent sa position
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

    // Comme au chapitre 9 : tout attribut interpolé en perspective est pré-divisé par w,
    // les normales comprises.
    const float uOverWA = vA.uv.u * vA.invW, vOverWA = vA.uv.v * vA.invW;
    const float uOverWB = vB.uv.u * vB.invW, vOverWB = vB.uv.v * vB.invW;
    const float uOverWC = vC.uv.u * vC.invW, vOverWC = vC.uv.v * vC.invW;
    const math::Vector3<float> nOverWA = vA.normal * vA.invW;
    const math::Vector3<float> nOverWB = vB.normal * vB.invW;
    const math::Vector3<float> nOverWC = vC.normal * vC.invW;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            const Pixel point{ x, y };
            const float edgeA = chapter_3_edge(pB, pC, point);
            const float edgeB = chapter_3_edge(pC, pA, point);
            const float edgeC = chapter_3_edge(pA, pB, point);

            // Hors du triangle
            if (edgeA < 0 || edgeB < 0 || edgeC < 0) continue;

            const float weightA = edgeA / area;
            const float weightB = edgeB / area;
            const float weightC = edgeC / area;

            // Test de profondeur
            const float z = weightA * vA.z + weightB * vB.z + weightC * vC.z;
            const int index = y * WIDTH + x;
            if (z >= depthBuffer[index]) continue;

            const float invW = weightA * vA.invW + weightB * vB.invW + weightC * vC.invW;
            const float u = (weightA * uOverWA + weightB * uOverWB + weightC * uOverWC) / invW;
            const float v = (weightA * vOverWA + weightB * vOverWB + weightC * vOverWC) / invW;

            // La moyenne de vecteurs unitaires n'est pas unitaire (elle "s'affaisse" vers le centre) :
            // on re-normalise, car les produits scalaires de l'éclairage supposent une normale unitaire.
            math::Vector3<float> normal = (nOverWA * weightA + nOverWB * weightB + nOverWC * weightC) / invW;
            normal.Normalize();

            // Diffus (chapitre 10), mais par pixel
            const float diffuse = std::max(0.0f, normal.Dot(light));
            const float brightness = CHAPTER_10_AMBIENT + (1.0f - CHAPTER_10_AMBIENT) * diffuse;

            // Spéculaire de Blinn-Phong : maximal quand la normale pointe le long du demi-vecteur
            const float specular = chapter_11_pow32(std::max(0.0f, normal.Dot(halfVector))) * CHAPTER_11_SPEC_STRENGTH;

            // Opaque fragments write depth; transparent fragments only test it.
            if (!transparentPass) depthBuffer[index] = z;

            Color3 texel = bilinear ? texture::sampleBilinear(meshTexture, u, v) : texture::sample(meshTexture, u, v);
            texel.r = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.r * brightness + 255.0f * specular), 0, 255));
            texel.g = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.g * brightness + 255.0f * specular), 0, 255));
            texel.b = static_cast<uint8_t>(std::clamp(static_cast<int>(texel.b * brightness + 255.0f * specular), 0, 255));

            if (transparentPass)
            {
                // frameBuffer uses ARGB8888 (0xAARRGGBB).
                const uint32_t dst = frameBuffer[index];
                const float a = std::clamp(opacity, 0.0f, 1.0f);
                const uint8_t dr = static_cast<uint8_t>((dst >> 16) & 0xFF);
                const uint8_t dg = static_cast<uint8_t>((dst >> 8) & 0xFF);
                const uint8_t db = static_cast<uint8_t>(dst & 0xFF);
                texel.r = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(texel.r * a + dr * (1.0f - a))), 0, 255));
                texel.g = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(texel.g * a + dg * (1.0f - a))), 0, 255));
                texel.b = static_cast<uint8_t>(std::clamp(static_cast<int>(std::lround(texel.b * a + db * (1.0f - a))), 0, 255));
            }
            add_pixel_to_buffer(Pixel{ x, y, texel });
        }
    }
}

// Même pipeline que le chapitre 10, avec une normale par sommet et un éclairage par pixel.
void chapter_11(const Mesh& mesh, const Texture& meshTexture,
    const std::vector<math::Vector3<float>>& vertexNormals, float seconds)
{
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(fovY, aspect, 0.1f, 100.0f);

    const math::Mat4<float> view = math::Mat4<float>::LookAt(
        math::Vector3<float>(0.0f, 0.0f, 3.0f),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f));

    const math::Mat4<float> model = math::Mat4<float>::Rotate(
        math::Quaternion::FromAxisAngle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f));
    const math::Mat4<float> modelView = model * view;

    // Même lumière qu'au chapitre 10.
    const math::Vector3<float> light = math::Vector3<float>(0.4f, 0.8f, 0.45f).Normalized();

    // La caméra est en z = 3 et regarde l'origine : depuis la surface, l'oeil est à peu près en +Z.
    // On garde cette direction constante, donc le demi-vecteur H = normalize(L + V) se calcule une fois par image.
    const math::Vector3<float> viewDirection(0.0f, 0.0f, 1.0f);
    const math::Vector3<float> halfVector = (light + viewDirection).Normalized();

    std::fill_n(depthBuffer, WIDTH * HEIGHT, std::numeric_limits<float>::max());

    std::vector<ShadedVertex> projected(mesh.vertices.size());
    std::vector<bool> visible(mesh.vertices.size(), false);

    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        const math::Vector3<float> cameraPos = modelView.MultiplyPointAffine(mesh.vertices[i]);
        const float w = -cameraPos.z;
        if (w <= 0.0001f) continue;

        const math::Vector3<float> ndc = projection.MultiplyPoint(cameraPos);
        projected[i].x = static_cast<int>((ndc.x + 1.0f) * 0.5f * WIDTH);
        projected[i].y = static_cast<int>((1.0f - ndc.y) * 0.5f * HEIGHT);
        projected[i].z = ndc.z;
        projected[i].invW = 1.0f / w;

        // La normale est une direction : on la tourne avec la matrice modèle, sans translation.
        projected[i].normal = model.MultiplyVector(vertexNormals[i]);
        visible[i] = true;
    }

    for (const Face& face : mesh.faces)
    {
        if (!visible[face.a] || !visible[face.b] || !visible[face.c]) continue;

        ShadedVertex vA = projected[face.a];
        ShadedVertex vB = projected[face.b];
        ShadedVertex vC = projected[face.c];

        // Backface culling (voir chapitre 9)
        if (chapter_3_edge(Pixel{ vA.x, vA.y }, Pixel{ vB.x, vB.y }, Pixel{ vC.x, vC.y }) >= 0.0f) continue;

        vA.uv = (face.uvA >= 0) ? mesh.uvs[face.uvA] : UV{};
        vB.uv = (face.uvB >= 0) ? mesh.uvs[face.uvB] : UV{};
        vC.uv = (face.uvC >= 0) ? mesh.uvs[face.uvC] : UV{};

        chapter_11_draw_triangle_phong(vA, vB, vC, meshTexture, light, halfVector, false);
    }
}


// Chapitre 12 : clipping sur le plan near + filtrage bilinéaire


// Passer à false pour revenir à l'échantillonnage "plus proche voisin" (en blocs) du chapitre 9.
constexpr bool CHAPTER_12_BILINEAR = true;

// F toggles the transparent rendering pass (alpha blending, no depth writes).
// This demo currently has one mesh, so the transparent pass renders that mesh alone.
bool gTransparencyEnabled = false;
constexpr float TRANSPARENT_OPACITY = 0.42f;

// Distance entre la caméra et Spot. Mettre 0.4f pour entrer dans la vache :
// la surface reste pleine jusqu'à la coupe, au lieu de perdre des triangles entiers.
constexpr float CHAPTER_12_CAMERA_DISTANCE = 3.0f;

// Un sommet en clip space (après la projection, AVANT la division par w),
// avec tous les attributs que le clipping doit interpoler et dont le rasterizer a besoin ensuite.
struct ClipVertex
{
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f; // position en clip space
    UV uv;                                        // coordonnée de texture
    math::Vector3<float> normal;                  // normale en espace monde
};

// p * M avec p = (x, y, z, 1), en gardant w : MultiplyPoint ferait déjà la division par w,
// et c'est justement ce qu'on veut éviter tant que le triangle n'est pas clippé.
ClipVertex chapter_12_to_clip_space(const math::Mat4<float>& matrix, const math::Vector3<float>& p)
{
    ClipVertex vertex;
    vertex.x = p.x * matrix(0, 0) + p.y * matrix(1, 0) + p.z * matrix(2, 0) + matrix(3, 0);
    vertex.y = p.x * matrix(0, 1) + p.y * matrix(1, 1) + p.z * matrix(2, 1) + matrix(3, 1);
    vertex.z = p.x * matrix(0, 2) + p.y * matrix(1, 2) + p.z * matrix(2, 2) + matrix(3, 2);
    vertex.w = p.x * matrix(0, 3) + p.y * matrix(1, 3) + p.z * matrix(2, 3) + matrix(3, 3);
    return vertex;
}

// Interpole TOUS les attributs : a * (1 - t) + b * t.
ClipVertex chapter_12_lerp(const ClipVertex& a, const ClipVertex& b, float t)
{
    ClipVertex result;
    result.x = a.x + t * (b.x - a.x);
    result.y = a.y + t * (b.y - a.y);
    result.z = a.z + t * (b.z - a.z);
    result.w = a.w + t * (b.w - a.w);
    result.uv.u = a.uv.u + t * (b.uv.u - a.uv.u);
    result.uv.v = a.uv.v + t * (b.uv.v - a.uv.v);
    result.normal = a.normal + (b.normal - a.normal) * t;
    return result;
}

// Distance signée au plan near, en unités du clip space. Positive = devant le plan near (on garde).
// Devant le near <=> z / w >= -1 <=> z + w >= 0 : le test marche sans diviser par w.
float chapter_12_near_distance(const ClipVertex& vertex)
{
    return vertex.z + vertex.w;
}

// Clipping de Sutherland-Hodgman d'un triangle contre le plan near.
// On parcourt les arêtes A->B, B->C, C->A et pour chacune on émet :
//  - les deux bouts devant       -> le bout B
//  - A devant, B derrière (sort) -> le point de croisement
//  - A derrière, B devant (entre)-> le point de croisement, puis B
//  - les deux derrière           -> rien
// Le polygone obtenu a 0, 3 ou 4 sommets ; un quad est découpé en 2 triangles (0,1,2) et (0,2,3).
// Écrit 0, 3 ou 6 sommets dans out et retourne le nombre de triangles (0, 1 ou 2).
int chapter_12_clip_triangle_near(const ClipVertex& vA, const ClipVertex& vB, const ClipVertex& vC, ClipVertex out[6])
{
    const ClipVertex triangle[3] = { vA, vB, vC };
    ClipVertex polygon[4]; // un triangle coupé par UN plan donne au plus 4 sommets
    int count = 0;

    for (int edge = 0; edge < 3; edge++)
    {
        const ClipVertex& a = triangle[edge];
        const ClipVertex& b = triangle[(edge + 1) % 3];
        const float distanceA = chapter_12_near_distance(a);
        const float distanceB = chapter_12_near_distance(b);
        const bool aInside = distanceA >= 0.0f;
        const bool bInside = distanceB >= 0.0f;

        // L'arête traverse le plan : le croisement est à la fraction t où la distance s'annule
        if (aInside != bInside) polygon[count++] = chapter_12_lerp(a, b, distanceA / (distanceA - distanceB));
        if (bInside) polygon[count++] = b;
    }

    if (count < 3) return 0;

    out[0] = polygon[0]; out[1] = polygon[1]; out[2] = polygon[2];
    if (count == 3) return 1;

    out[3] = polygon[0]; out[4] = polygon[2]; out[5] = polygon[3];
    return 2;
}

// Un triangle déjà clippé (entièrement devant le plan near, donc w > 0) :
// division par w, viewport, backface culling, puis le rasterizer Phong du chapitre 11.
void chapter_12_project_and_draw(const ClipVertex triangle[3], const Texture& meshTexture,
    const math::Vector3<float>& light, const math::Vector3<float>& halfVector,
    bool transparentPass, float opacity)
{
    ShadedVertex projected[3];
    for (int k = 0; k < 3; k++)
    {
        const float invW = 1.0f / triangle[k].w;
        projected[k].x = static_cast<int>((triangle[k].x * invW + 1.0f) * 0.5f * WIDTH);
        projected[k].y = static_cast<int>((1.0f - triangle[k].y * invW) * 0.5f * HEIGHT);
        projected[k].z = triangle[k].z * invW;
        projected[k].invW = invW;
        projected[k].uv = triangle[k].uv;
        projected[k].normal = triangle[k].normal;
    }

    // Backface culling (voir chapitre 9)
    if (chapter_3_edge(Pixel{ projected[0].x, projected[0].y }, Pixel{ projected[1].x, projected[1].y },
        Pixel{ projected[2].x, projected[2].y }) >= 0.0f) return;

    chapter_11_draw_triangle_phong(projected[0], projected[1], projected[2], meshTexture,
        light, halfVector, CHAPTER_12_BILINEAR, transparentPass, opacity);
}

// Même rendu que le chapitre 11, mais le pipeline est réorganisé autour du clipping :
//  1) par sommet du mesh : seulement le passage en clip space et la normale monde (pas de division) ;
//  2) par triangle : clipping near -> 0, 1 ou 2 triangles, puis division + viewport + culling + raster.
// Le clipping peut créer de NOUVEAUX sommets (les points de croisement), c'est pour ça que
// la division par w ne peut plus se faire une fois par sommet du mesh.
void chapter_12(const Mesh& mesh, const Texture& meshTexture,
    const std::vector<math::Vector3<float>>& vertexNormals, float seconds,
    bool transparentPass = false)
{
    const float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    const float fovY = 50.0f * std::numbers::pi_v<float> / 180.0f;
    const math::Mat4<float> projection = math::Mat4<float>::Perspective(fovY, aspect, 0.1f, 100.0f);

    const math::Mat4<float> view = math::Mat4<float>::LookAt(
        math::Vector3<float>(0.0f, 0.0f, CHAPTER_12_CAMERA_DISTANCE),
        math::Vector3<float>(0.0f, 0.0f, 0.0f),
        math::Vector3<float>(0.0f, 1.0f, 0.0f));

    const math::Mat4<float> model = math::Mat4<float>::Rotate(
        math::Quaternion::FromAxisAngle(math::Vector3<float>(0.0f, 1.0f, 0.0f), seconds * 0.7f));

    // Vecteurs lignes (p * M) : modèle, puis vue, puis projection.
    const math::Mat4<float> modelViewProjection = model * view * projection;

    const math::Vector3<float> light = math::Vector3<float>(0.4f, 0.8f, 0.45f).Normalized();
    const math::Vector3<float> viewDirection(0.0f, 0.0f, 1.0f);
    const math::Vector3<float> halfVector = (light + viewDirection).Normalized();

    std::fill_n(depthBuffer, WIDTH * HEIGHT, std::numeric_limits<float>::max());

    // 1) Par sommet : clip space + normale monde. Plus besoin du tableau "visible" :
    // le clipping gère rigoureusement ce qui est derrière la caméra.
    std::vector<ClipVertex> clipVertices(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        clipVertices[i] = chapter_12_to_clip_space(modelViewProjection, mesh.vertices[i]);
        clipVertices[i].normal = model.MultiplyVector(vertexNormals[i]);
    }

    // 2) Par triangle. For transparency, sort faces back-to-front (approximation
    // by average clip-space w); transparent fragments do not update the Z-buffer.
    std::vector<size_t> faceOrder(mesh.faces.size());
    for (size_t i = 0; i < faceOrder.size(); ++i) faceOrder[i] = i;
    if (transparentPass)
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
        const int triangleCount = chapter_12_clip_triangle_near(vA, vB, vC, clipped);
        for (int t = 0; t < triangleCount; t++)
        {
            chapter_12_project_and_draw(&clipped[t * 3], meshTexture, light, halfVector,
                transparentPass, transparentPass ? TRANSPARENT_OPACITY : 1.0f);
        }
    }
}


// Renderer custom functions

// Clears the frame, draws the current chapter, then sends the framebuffer to the screen.
void update_render(SDL_Renderer* renderer, SDL_Texture* texture, float seconds, const Mesh& mesh, const Texture& meshTexture,
    const std::vector<math::Vector3<float>>& faceNormals, const std::vector<math::Vector3<float>>& vertexNormals)
{
    // Clear the CPU framebuffer.
    std::fill_n(frameBuffer, WIDTH * HEIGHT, Color3{ 16, 16, 24, 255 }.toRGB());

    // Draw the current chapter. Chapter 12 is the default merged result.
    // Pour revenir au chapitre 8, décommentez son appel et commentez chapter_12.
    int drawnTriangles = 0;
    int culledTriangles = 0;
    //chapter_8(mesh, seconds, drawnTriangles, culledTriangles);
    //chapter_9(mesh, meshTexture, seconds);
    //chapter_10(mesh, meshTexture, faceNormals, seconds);
    //chapter_11(mesh, meshTexture, vertexNormals, seconds);
    chapter_12(mesh, meshTexture, vertexNormals, seconds, gTransparencyEnabled);

    // Upload the CPU framebuffer, then draw readable UI text on top using SDL3's
    // built-in debug font (no additional TTF dependency required).
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(texture, nullptr, frameBuffer, WIDTH * sizeof(uint32_t));
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, 12.0f, 12.0f, "3Dzavr CPU Renderer | Chapter 12");
    SDL_RenderDebugText(renderer, 12.0f, 26.0f, gTransparencyEnabled
        ? "Pass: TRANSPARENT | alpha = 0.42 | press T to switch"
        : "Pass: OPAQUE | press T to switch to transparency");
    SDL_RenderDebugText(renderer, 12.0f, 40.0f, "ESC: quit");
    SDL_RenderPresent(renderer);
}


// Main function

int main()
{
    // Load the mesh and its texture once, before the loop (chapters 6 and 9)
    Mesh spot;
    if (!mesh::loadObj("assets/spot.obj", spot)) return EXIT_FAILURE;
    std::cout << "Loaded spot.obj:\t" << spot.vertices.size() << " vertices, " << spot.uvs.size() << " uvs, " << spot.faces.size() << " faces\n";

    Texture spotTexture;
    if (!texture::loadPng("assets/spot_texture.png", spotTexture)) return EXIT_FAILURE;
    std::cout << "Loaded spot_texture.png:\t" << spotTexture.width << "x" << spotTexture.height << " texels\n";

    // Les normales ne dépendent pas de la rotation : on les calcule une seule fois.
    const std::vector<math::Vector3<float>> faceNormals = chapter_10_compute_face_normals(spot);

    // Chapitres 11 et 12 : une normale par sommet, moyenne des normales des faces voisines.
    const std::vector<math::Vector3<float>> vertexNormals = chapter_11_compute_vertex_normals(spot, faceNormals);

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
    char   title[160];

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
                else if (event.key.key == SDLK_T)
                {
                    gTransparencyEnabled = !gTransparencyEnabled;
                }
            }
        }

        // Elapsed time, used by the chapters to animate.
        const float seconds = static_cast<float>(
            static_cast<double>(SDL_GetPerformanceCounter() - start)
            / static_cast<double>(freq));

        // Draw the frame (the current chapter is picked inside update_render).
        update_render(renderer, texture, seconds, spot, spotTexture, faceNormals, vertexNormals);

        // FPS in the window title, refreshed once per second.
        const Uint64 now = SDL_GetPerformanceCounter();
        fps_accum += static_cast<double>(now - last) / static_cast<double>(freq);
        fps_count++;
        last = now;

        if (fps_accum >= 1.0)
        {
            const double fps = fps_count / fps_accum;
            const double ms = 1000.0 * fps_accum / fps_count;
            SDL_snprintf(title, sizeof(title), "Chapitre 12 - %s | %s | %.1f FPS (%.2f ms)",
                gTransparencyEnabled ? "Transparency" : "Opaque pass",
                CHAPTER_12_BILINEAR ? "bilineaire" : "plus proche", fps, ms);
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