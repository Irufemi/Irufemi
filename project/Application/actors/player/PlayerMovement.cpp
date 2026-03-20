#include "PlayerMovement.h"
#include <Windows.h>

void PlayerMovement::Initialize() {
    velocity_ = { 0.0f, 0.0f, 0.0f };
    isGrounded_ = true;
    dodgeCooldownTimer_ = 0;
    dodgeDurationTimer_ = 0;
    dodgeDirection_ = { 0.0f, 0.0f, 0.0f };
}

void PlayerMovement::UpdateTimers() {
    if (dodgeCooldownTimer_ > 0) {
        dodgeCooldownTimer_--;
    }
}

void PlayerMovement::Update(InputManager* input, bool isCharging, bool isKarakuriCharged,
    Vector3& translate, const Vector3& rotate, int& invincibleTimer) {
    Vector3 move = { 0.0f, 0.0f, 0.0f };

    // 回避行動中の強制移動処理（通常の移動入力を無視する）
    if (dodgeDurationTimer_ > 0) {
        translate.x += dodgeDirection_.x * kDodgeSpeed;
        translate.z += dodgeDirection_.z * kDodgeSpeed;

        // フィールド外に出ないように制限
        if (translate.x > kFieldRangeX)  translate.x = kFieldRangeX;
        if (translate.x < -kFieldRangeX) translate.x = -kFieldRangeX;
        if (translate.z > kFieldRangeZ)  translate.z = kFieldRangeZ;
        if (translate.z < -kFieldRangeZ) translate.z = -kFieldRangeZ;

        dodgeDurationTimer_--;
        return; // 回避中は通常の移動やジャンプ処理を行わない
    }

    if (!isCharging) {
        if (input->IsKeyDown('W')) move.z += 1.0f;
        if (input->IsKeyDown('S')) move.z -= 1.0f;
        if (input->IsKeyDown('A')) move.x -= 1.0f;
        if (input->IsKeyDown('D')) move.x += 1.0f;
    }

    // 通常の移動方向の計算
    float moveX = 0.0f;
    float moveZ = 0.0f;
    if (move.x != 0.0f || move.z != 0.0f) {
        move = Math::Normalize(move);
        float sinY = std::sin(rotate.y);
        float cosY = std::cos(rotate.y);
        moveX = move.x * cosY + move.z * sinY;
        moveZ = -move.x * sinY + move.z * cosY;

        translate.x += moveX * kMoveSpeed;
        translate.z += moveZ * kMoveSpeed;

        if (translate.x > kFieldRangeX)  translate.x = kFieldRangeX;
        if (translate.x < -kFieldRangeX) translate.x = -kFieldRangeX;
        if (translate.z > kFieldRangeZ)  translate.z = kFieldRangeZ;
        if (translate.z < -kFieldRangeZ) translate.z = -kFieldRangeZ;
    }

    if (isGrounded_) {
        // Spaceキーの処理を、からくりチャージ中かどうかで分岐
        if (!isCharging && input->IsKeyPressed(VK_SPACE)) {
            if (isKarakuriCharged) {
                // からくりチャージ中：回避アクション
                if (dodgeCooldownTimer_ <= 0) {
                    dodgeCooldownTimer_ = kDodgeCooldownTime; // クールタイム2秒
                    dodgeDurationTimer_ = kDodgeDurationTime; // 回避モーションの時間
                    invincibleTimer = kDodgeDurationTime;    // 既存の無敵タイマーを利用して回避中を無敵に

                    // 移動入力があればその方向へ、なければ向いている方向（前）へ回避
                    if (move.x != 0.0f || move.z != 0.0f) {
                        dodgeDirection_ = { moveX, 0.0f, moveZ };
                    } else {
                        float sinY = std::sin(rotate.y);
                        float cosY = std::cos(rotate.y);
                        dodgeDirection_ = { sinY, 0.0f, cosY };
                    }
                    dodgeDirection_ = Math::Normalize(dodgeDirection_);
                }
            } else {
                // 通常時：ジャンプ
                velocity_.y = kJumpForce;
                isGrounded_ = false;
            }
        }
    } else {
        velocity_.y -= kGravity;
        translate.y += velocity_.y;
        if (translate.y <= 0.0f) {
            translate.y = 0.0f;
            velocity_.y = 0.0f;
            isGrounded_ = true;
        }
    }
}