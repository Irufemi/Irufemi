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
    float initialScaleY_ = 1.0f;
    float rotationInterpolationSpeed_ = 5.0f; // 回転の追従速度（大きいほど速い）

    // --- 調整用パラメータ (ここをいじるだけで挙動が変わります) ---

    // 【フェーズ時間設定】
    float squatTime_ = 0.6f;        // 屈伸にかける時間
    float holdTime_ = 1.2f;         // 溜め（微振動）の時間
    float jumpTime_ = 0.35f;        // 飛び上がりにかける時間
    float hoverTime_ = 2.5f;        // プレイヤー頭上での待機時間
    float recoveryTime_ = 1.2f;     // 着地後の硬直時間
    float landSquatScale_ = 0.35f;  // 着地した瞬間にどれだけ潰れるか（小さいほど深く潰れる）
    float landSquatDownTime_ = 0.4f; // 地面についてから最大まで「潰れる」時間
    float landSquatHoldTime_ = 0.2f;  // 最大まで潰れたまま「耐える」時間
    float landRiseTime_ = 3.0f;       // 元のサイズに「戻る」時間

    // 【演出の強さ設定】
    float maxSquatScale_ = 0.5f;    // 最大まで縮んだ時のスケール倍率
    float jumpStretchScale_ = 2.0f; // ジャンプした瞬間の縦伸び倍率

    float squatShakeStrength_ = 0.5f; // 屈伸中の振動の最大幅
    float holdShakeStrength_ = 0.15f; // 溜め中の微振動の幅
    float holdShakeSpeed_ = 180.0f;   // 溜め中の微振動の速さ

    // 【移動・高さ設定】
    float jumpUpSpeed_ = 4.0f;      // ジャンプ時の上昇量(フレーム毎)
    float dropSpeed_ = 1.5f;        // 落下速度
    float stompHeight_ = 40.0f;     // 攻撃を開始する高度
    float groundY_ = 3.0f;          // 地面の高さ判定
};