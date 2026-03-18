#include "EnemyAnimState_Idle.h"
#include "Enemy.h"
#include <cmath>

void EnemyAnimState_Idle::Enter(Enemy* enemy) {
    // 待機に入るときはタイマーをリセットしなくても良い（動きを継続させるため）
}

void EnemyAnimState_Idle::Update(Enemy* enemy, Player* player, float deltaTime) {
    timer_ += deltaTime;

    // --- 元の UpdateIdle() のロジックを完全移植 ---
    float ls = lerpSpeed_;

    // 各首の呼吸（縦揺れ）
    auto ApplyIdleBreath = [&](Vector3& offset, float phase) {
        float waveY = std::sin(timer_ * breathSpeed_ + phase) * breathHeight_;
        // X, Zは中心(0)に戻るように、Yは波の動きに合わせて補間
        offset.x += (0.0f - offset.x) * ls;
        offset.y += (waveY - offset.y) * ls;
        offset.z += (0.0f - offset.z) * ls;
        };

    ApplyIdleBreath(enemy->GetHeadMidOffset(), 0.0f);
    ApplyIdleBreath(enemy->GetHeadLeftOffset(), phaseOffset_);
    ApplyIdleBreath(enemy->GetHeadRightOffset(), phaseOffset_ * 2.0f);

    // 胴体のうねり
    for (int i = 0; i < 3; ++i) {
        float bodyWave = std::sin(timer_ * breathSpeed_ + (i * phaseOffset_)) * bodyWaveHeight_;
        enemy->GetBodyOffset(i).y += (bodyWave - enemy->GetBodyOffset(i).y) * ls;
    }

    // ゆっくりと自転
    enemy->GetGlobalTransform().rotate.y += idleRotationSpeed_;
}

void EnemyAnimState_Idle::Exit(Enemy* enemy) {}