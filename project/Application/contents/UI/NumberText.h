#pragma once

#include "2D/Sprite.h"
#include "math/Vector2.h"
#include "math/Vector4.h"
#include <memory>
#include <string>
#include <array>

class Camera;

class NumberText {
public:
    void Initialize(Camera* camera, const std::string& textureBasePath, const std::string& fileExtension, float numberWidth, float numberHeight);
    void Draw(int number);

    void SetPosition(const Vector2& position);
    void SetSize(float width, float height);
    void SetColor(const Vector4& color);

private:
    Camera* camera_ = nullptr;
    // 0-9までの数字スプライトを保持する配列
    std::array<std::unique_ptr<Sprite>, 10> numberSprites_;
    Vector2 position_{ 0.0f, 0.0f };
    Vector2 spriteSize_{ 0.0f, 0.0f };
};