#pragma once

#include "Engine/Core/Math/Math.h"
#include "Irufemi.h"
#include <cmath>

class InputManager;

class PlayerMovement {
public:
    PlayerMovement() = default;
    ~PlayerMovement() = default;

    // 初期化
    void Initialize();

    // 毎フレームのタイマー更新（Updateの最初に呼ぶ）
    void UpdateTimers();

    // 移動と回避のメイン処理
    // isCharging: Eキー等によるチャージ中かどうか
    // isKarakuriCharged: からくりチャージ状態かどうか
    // translate: プレイヤーの座標（参照渡しで更新）
    // rotate: プレイヤーの回転（Y軸の向きを使用）
    // invincibleTimer: プレイヤーの無敵時間（回避成功時などに加算）
    void Update(InputManager* input, bool isCharging, bool isKarakuriCharged,
        Vector3& translate, const Vector3& rotate, int& invincibleTimer);

    // ゲッター
    int GetDodgeCooldownTimer() const { return dodgeCooldownTimer_; }
    int GetDodgeDurationTimer() const { return dodgeDurationTimer_; }
    int GetMaxDodgeCooldownTime() const { return kDodgeCooldownTime; }
    const Vector3& GetVelocity() const { return velocity_; }
    bool IsGrounded() const { return isGrounded_; }

    float* GetDodgeSpeedPtr() { return &dodgeSpeed_; }
    float* GetDodgeSpeedNormalMultiplierPtr() { return &dodgeSpeedNormalMultiplier_; }

    void ResetDodgeCooldown() { dodgeCooldownTimer_ = 0; }

private:
    // --- 移動・ジャンプ関連変数 ---
    Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };
    bool isGrounded_ = true;

    // --- 回避行動用変数 ---
    int dodgeCooldownTimer_ = 0;         // 回避のクールタイム
    const int kDodgeCooldownTime = 120;  // クールタイム2秒（60FPS想定）
    int dodgeDurationTimer_ = 0;         // 回避行動自体の持続時間
    const int kDodgeDurationTime = 30;   // 回避時間（約0.5秒）
    Vector3 dodgeDirection_ = { 0.0f, 0.0f, 0.0f }; // 回避する方向
    float dodgeSpeed_ = 1.2f;            // 回避の移動速度
    float dodgeSpeedNormalMultiplier_ = 0.4f; // 通常時の回避速度倍率

    // --- パラメータ ---
    const float kMoveSpeed = 0.2f;
    const float kJumpForce = 0.25f;
    const float kGravity = 0.02f;

    // --- フィールドの境界 ---
    const float kFieldRangeX = 100.0f;
    const float kFieldRangeZ = 100.0f;
};