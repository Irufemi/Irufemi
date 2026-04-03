#pragma once
#include "IEnemyAnimationState.h"
#include "core/math/Vector3.h"

class EnemyAnimState_Stomp : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    bool IsFinished() const override { return hasFinishedAttack_; }

private:
    float attackTimer_ = 0.0f;
    bool hasFinishedAttack_ = false;
    bool hasTeleported_ = false;
    bool hasHitGround_ = false;

    // --- 調整用パラメータ ---
    float anticipationTime_ = 3.0f; // 地上での予兆（プルプル）
    float hoverTime_ = 2.5f;        // プレイヤー頭上でのタメ（プルプル）
    float dropSpeed_ = 1.5f;        // 落下速度倍率
    float recoveryTime_ = 1.2f;     // 着地後の硬直
    float stompHeight_ = 40.0f;     // どの高さまでテレポートするか
    float initialScaleY_ = 1.0f;
};