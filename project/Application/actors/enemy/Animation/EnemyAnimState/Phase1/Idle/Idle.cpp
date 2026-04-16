#include "Idle.h"
#include "Enemy.h"
#include <cmath>

void Idle::Update(Enemy* enemy, Player* player, float deltaTime) {
    timer_ += deltaTime;
    float ls = lerpSpeed_;

    auto ApplyIdleBreath = [&](Vector3& offset, float phase) {
        float waveY = std::sin(timer_ * breathSpeed_ + phase) * breathHeight_;
        offset.x += (0.0f - offset.x) * returnSpeed_;
        offset.y += (waveY - offset.y) * ls;
        offset.z += (0.0f - offset.z) * returnSpeed_;
        };

    ApplyIdleBreath(enemy->GetHeadMidOffset(), 0.0f);
    ApplyIdleBreath(enemy->GetHeadLeftOffset(), phaseOffset_);
    ApplyIdleBreath(enemy->GetHeadRightOffset(), phaseOffset_ * 2.0f);

    for (int i = 0; i < 3; ++i) {
        float waveBodyY = std::sin(timer_ * breathSpeed_ - (float)(i + 1) * phaseOffset_) * bodyWaveHeight_;
        enemy->GetBodyOffset(i).y += (waveBodyY - enemy->GetBodyOffset(i).y) * ls;
        enemy->GetBodyOffset(i).x += (0.0f - enemy->GetBodyOffset(i).x) * returnSpeed_;
    }

    enemy->GetGlobalTransform().rotate.y += idleRotationSpeed_;
    enemy->GetGlobalTransform().rotate.x += (0.0f - enemy->GetGlobalTransform().rotate.x) * returnSpeed_;
}