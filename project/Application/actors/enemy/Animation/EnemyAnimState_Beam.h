#pragma once
#include "IEnemyAnimationState.h"
#include "core/math/Vector3.h"

class EnemyAnimState_Beam : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    // インターフェースのオーバーライド
    bool IsFinished() const override { return hasFinishedAttack_; }
    bool IsFiring() const override { return isFiring_; }

private:
    // ヘルパー関数：角度正規化
    float NormalizeAngle(float angle);

private:
    // --- 内部状態フラグ ---
    float attackTimer_ = 0.0f;
    bool isLockedOn_ = false;
    bool isFiring_ = false;
    bool hasFinishedAttack_ = false;
    Vector3 lockedTargetPos_ = { 0, 0, 0 };

    // --- EnemyAnimation.h から移植した全パラメータ ---
    float chargeTime_ = 3.0f;
    float anticipationTime_ = 1.2f;
    float fireTime_ = 2.5f;
    float stunTime_ = 1.0f;
    float recoveryTime_ = 1.5f;

    float gatherStrength_ = 1.8f;
    float shakeStrength_ = 0.2f;
    float stunShakeStrength_ = 0.5f;
    float exhaustionDepth_ = -1.5f;

    float returnSpeed_ = 0.03f;
    float lerpSpeed_ = 0.1f;
};