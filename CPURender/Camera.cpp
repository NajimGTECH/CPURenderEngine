#include "Camera.h"
#include <cmath>

Vector3 WorldToCamera(const Vector3& point, const Camera& camera)
{
    // Translate the point relative to the camera
    const float dx = point.x - camera.position.x;
    const float dy = point.y - camera.position.y;
    const float dz = point.z - camera.position.z;

    // Apply inverse yaw rotation
    const float cosYaw = std::cos(camera.yaw);
    const float sinYaw = std::sin(camera.yaw);

    const float rotatedX = cosYaw * dx - sinYaw * dz;
    const float rotatedZ = sinYaw * dx + cosYaw * dz;

    // Apply inverse pitch rotation
    const float cosPitch = std::cos(camera.pitch);
    const float sinPitch = std::sin(camera.pitch);

    const float rotatedY = cosPitch * dy + sinPitch * rotatedZ;
    const float finalZ = -sinPitch * dy + cosPitch * rotatedZ;

    return { rotatedX, rotatedY, finalZ };
}