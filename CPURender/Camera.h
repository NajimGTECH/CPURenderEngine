#pragma once

struct Vector3
{
    float x;
    float y;
    float z;
};

struct Camera
{
    Vector3 position{ 0.0f, 0.0f, 0.0f };

    float yaw = 0.0f;
    float pitch = 0.0f;

    bool isLooking = false;
};

constexpr float MOUSE_SENSITIVITY = 0.005f;
constexpr float MAX_PITCH = 1.5f;

Vector3 WorldToCamera(const Vector3& point, const Camera& camera);