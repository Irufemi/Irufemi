#pragma once
#include <memory>
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"

class Camera;
class PlaneClass;
class Sprite;
class Player;
class IrufemiEngine;

class PlayerHPBar {
public:
    PlayerHPBar() = default;
    ~PlayerHPBar() = default;

    void Initialize(Camera* camera, IrufemiEngine* engine);
    void Update(const Player* player, const Camera* camera, bool isFirstPerson);
    void Draw3D(bool isUI = false);
    void Draw2D();

private:
    void UpdateBarColor(float hpRatio);

    std::unique_ptr<PlaneClass> barFrame_;
    std::unique_ptr<PlaneClass> barBg_;
    std::unique_ptr<PlaneClass> barFill_;

    std::unique_ptr<Sprite> spriteFrame_;
    std::unique_ptr<Sprite> spriteBg_;
    std::unique_ptr<Sprite> spriteFill_;

    float barMaxWidth3D_ = 0.0f;
    float barHeight3D_ = 0.0f;

    float barMaxWidth2D_ = 0.0f;
    float barHeight2D_ = 0.0f;
    float barX2D_ = 0.0f;
    float barY2D_ = 0.0f;

    float displayRatio_ = 1.0f;
};
