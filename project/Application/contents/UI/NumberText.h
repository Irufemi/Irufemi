#pragma once

#include "2D/Sprite.h"
#include "math/Vector2.h"
#include "math/Vector4.h"
#include <memory>
#include <string>

class Camera;

class NumberText {
public:
    void Initialize(Camera* camera, const std::string& texturePath, float numberWidth, float numberHeight);
    void Draw(int number);

    void SetPosition(const Vector2& position);
    void SetSize(float width, float height);
    void SetColor(const Vector4& color);

private:
    Camera* camera_ = nullptr;
    std::unique_ptr<Sprite> numberSprite_ = nullptr;
    std::string texturePath_;
    float numberWidth_ = 0.0f;
    float numberHeight_ = 0.0f;
    Vector2 position_{ 0.0f, 0.0f };
    Vector2 spriteSize_{ 0.0f, 0.0f };
};