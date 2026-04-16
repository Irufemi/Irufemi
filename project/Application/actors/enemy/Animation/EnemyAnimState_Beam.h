#pragma once
#include "IEnemyAnimationState.h"
#include "core/math/Vector3.h"

class EnemyAnimState_Beam : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    bool IsFinished() const override { return hasFinishedAttack_; }
    bool IsFiring() const override { return isFiring_; }

private:
    // --- 内部タイマーとフラグ ---
    float attackTimer_ = 0.0f;
    float totalTimer_ = 0.0f; // 振動用の継続時間
    bool isLockedOn_ = false;
    bool isFiring_ = false;
    bool hasFinishedAttack_ = false;
    Vector3 lockedTargetPos_ = { 0, 0, 0 };

    // --- 【完全復元】元の演出パラメータ ---
    float returnSpeed_ = 0.03f;
    float lerpSpeed_ = 0.1f;

    float chargeTime_ = 3.0f;
    float anticipationTime_ = 1.2f;
    float fireTime_ = 2.5f;
    float stunTime_ = 1.0f;
    float recoveryTime_ = 1.0f;

    float beamRotateSpeed_ = 0.1f;
    float beamThicknessFire_ = 12.0f; // ここも元の値
    float beamExpandScale_ = 2.5f;
    float headExtensionY_ = 16.0f;   // ここも元の値
    float fadeOutStartThreshold_ = 0.85f;

    float fireLeanAngleX_ = 0.25f;

    float shakeBaseSpeed_ = 95.0f;   // 元の激しい速度
    float chargeHeadShake_ = 0.35f;
    float chargeBodyShake_ = 0.15f;
    float fireHeadShake_ = 0.75f;
    float fireBodyShake_ = 0.35f;
    float stunShakeStrength_ = 0.1f;

    float exhaustionDepth_ = -2.5f;
};