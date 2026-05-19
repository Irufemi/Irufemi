#pragma once
#include "../../../IEnemyAnimationState.h"
#include "core/math/Vector3.h"

/**
 * @brief 第2形態の爆弾投擲ステート
 */
class Phase2_Bomb : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    void SetHeadIndex(int index) { headIndex_ = index; }
    bool IsFinished() const override { return isFinished_; }

private:
    float timer_ = 0.0f;
    float globalTimer_ = 0.0f;
    bool isFinished_ = false;
    int headIndex_ = 1;         // 管理する首のインデックス

    // 座標系
    Vector3 basePos_ = { 0, 0, 0 }; // シェイクの中心座標

    // --- 調整用パラメータ ---
    // 時間
    const float kChargeTime = 1.0f;      // 投げる前の溜め
    const float kThrowTime = 1.5f;       // 投げモーション完了

    // シェイク（震え）
    float shakeSpeedCharge_ = 50.0f;     
    float kShakeStrength = 0.2f;         
    float throwHeightOffset_ = 2.0f;     // 投げる瞬間の振りかぶり/振り下ろし
    
    bool hasThrown_ = false;
};
