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
    float NormalizeAngle(float angle);

private:
    // --- 状態フラグ ---
    float attackTimer_ = 0.0f;
    float totalTime_ = 0.0f; // std::sin用
    bool isLockedOn_ = false;
    bool isFiring_ = false;
    bool hasFinishedAttack_ = false;
    Vector3 lockedTargetPos_ = { 0, 0, 0 };

    // --- 【復元】調整用パラメータ (ここを変えれば以前と同じ調整が可能です) ---
    float chargeTime_ = 3.0f;
    float anticipationTime_ = 1.2f;
    float fireTime_ = 2.5f;
    float stunTime_ = 1.0f;
    float recoveryTime_ = 1.5f;

    float headExtensionY_ = 0.5f;      // 頭の高さ補正
    float fireLeanAngleX_ = 0.4f;       // 前傾角度
    float beamRotateSpeed_ = 0.05f;     // ビーム追尾速度
    float shakeBaseSpeed_ = 40.0f;      // 振動速度の基本値

    float chargeHeadShake_ = 0.15f;     // チャージ時の頭の揺れ
    float chargeBodyShake_ = 0.08f;     // チャージ時の体の揺れ
    float fireHeadShake_ = 0.25f;       // 本射時の頭の揺れ
    float fireBodyShake_ = 0.15f;       // 本射時の体の揺れ
    float stunShakeStrength_ = 0.5f;    // 硬直時の揺れ

    float beamThicknessFire_ = 0.8f;    // ビームの基本太さ
    float fadeOutStartThreshold_ = 0.7f;// 膨張開始タイミング
    float beamExpandScale_ = 2.0f;      // 膨張率

    float exhaustionDepth_ = -1.2f;     // ぐったりする深さ
    float returnSpeed_ = 0.03f;         // 姿勢戻り速度
    float lerpSpeed_ = 0.1f;            // 補間速度
};