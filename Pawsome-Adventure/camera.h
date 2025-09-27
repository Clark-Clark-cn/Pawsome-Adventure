#pragma once
#include "vector2.h"

class Camera
{
public:
    Camera() = default;
    ~Camera() = default;

    void setSize(const Vector2 size)
    {
        this->size = size;
    }
    const Vector2 &getSize() const
    {
        return size;
    }

    void setPosition(const Vector2 postion)

    {
        this->position = position;
    }

    const Vector2 &getPosition() const
    {
        return position;
    }

    void lookAt(const Vector2 &target)
    {
        position = target - size / 2.0f;
    }

private:
    Vector2 size;
    Vector2 position;
};
