#pragma once
#include "../../../IEnemyAnimationState.h"
#include "core/math/Vector3.h"

/**
 * @brief 待機ステート
 */
class Phase1_Idle : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override {}
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override {}
    bool IsFinished() const override { return true; }

private:
    float timer_ = 0.0f;
    float idleRotationSpeed_ = 0.005f;
    float breathSpeed_ = 2.0f;
    float breathHeight_ = 0.25f;
    float bodyWaveHeight_ = 0.15f;
    float phaseOffset_ = 0.6f;
    float lerpSpeed_ = 0.1f;
    float returnSpeed_ = 0.03f;
};