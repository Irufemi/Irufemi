#pragma once
#include <memory>
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"

class Camera;
class PlaneClass;
class Player;

class PlayerHPBar {
public:
    PlayerHPBar() = default;
    ~PlayerHPBar() = default;

    void Initialize(Camera* camera);
    void Update(const Player* player, const Camera* camera, bool isFirstPerson);
    void Draw();

private:
    void UpdateBarColor(float hpRatio);

    std::unique_ptr<PlaneClass> barFrame_;
    std::unique_ptr<PlaneClass> barBg_;
    std::unique_ptr<PlaneClass> barFill_;

    float barMaxWidth_ = 0.0f;
    float barHeight_ = 0.0f;

    float displayRatio_ = 1.0f;
};
