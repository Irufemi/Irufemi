#include "RailMechanics/RailShooterPlayerComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"
#define DIRECTINPUT_VERSION 0x0800
#include <algorithm>
#include <cmath>
#include <dinput.h>

void RailShooterPlayerComponent::OnRegisterProperties() {
    RegisterProperty("XYSpeed", &xySpeed_);
    RegisterProperty("MoveLimitMin", &moveLimitMin_);
    RegisterProperty("MoveLimitMax", &moveLimitMax_);
}

void RailShooterPlayerComponent::Update() {
    if (!gameObject_)
        return;

    // 1フレームの経過時間
    float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (deltaTime <= 0.0f) {
        return;
    }

    auto transform = GetTransform();
    if (!transform)
        return;

    // --- キー入力による上下左右の回避運動 ---
    auto* input = BaseModel::GetIrufemiEngine()->GetInputManager();
    Irufemi::Vector3 moveDir = {0.0f, 0.0f, 0.0f};

    // WASD または 矢印キーで移動方向を入力 (長押し判定のため IsKeyDownDIK を使用)
    if (input->IsKeyDownDIK(DIK_W) || input->IsKeyDownDIK(DIK_UP))
        moveDir.y += 1.0f;
    if (input->IsKeyDownDIK(DIK_S) || input->IsKeyDownDIK(DIK_DOWN))
        moveDir.y -= 1.0f;
    if (input->IsKeyDownDIK(DIK_A) || input->IsKeyDownDIK(DIK_LEFT))
        moveDir.x -= 1.0f;
    if (input->IsKeyDownDIK(DIK_D) || input->IsKeyDownDIK(DIK_RIGHT))
        moveDir.x += 1.0f;

    // ゲームパッド（左スティック）の入力
    float padX = input->GetLeftStickX();
    float padY = input->GetLeftStickY();
    if (std::abs(padX) > 0.1f)
        moveDir.x += padX;
    if (std::abs(padY) > 0.1f)
        moveDir.y += padY;

    // 斜め移動したときに移動速度が速くならないように、ベクトルの長さを1に抑える
    float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
    if (len > 1.0f) {
        moveDir.x /= len;
        moveDir.y /= len;
        len = 1.0f;
    }

    // --- 慣性と速度の計算 ---
    if (len > 0.1f) {
        // 入力がある場合、加速度を足して加速
        currentVelocity_.x += moveDir.x * acceleration_ * deltaTime;
        currentVelocity_.y += moveDir.y * acceleration_ * deltaTime;

        // 最高速度でクリップ
        float vLen = std::sqrt(currentVelocity_.x * currentVelocity_.x + currentVelocity_.y * currentVelocity_.y);
        if (vLen > maxSpeed_) {
            currentVelocity_.x = (currentVelocity_.x / vLen) * maxSpeed_;
            currentVelocity_.y = (currentVelocity_.y / vLen) * maxSpeed_;
        }
    } else {
        // 入力がない場合、摩擦（減衰）で急制動
        currentVelocity_.x = std::lerp(currentVelocity_.x, 0.0f, friction_ * deltaTime);
        currentVelocity_.y = std::lerp(currentVelocity_.y, 0.0f, friction_ * deltaTime);
    }

    // レール中心からのズレ幅（オフセット値）を更新
    currentOffset_.x += currentVelocity_.x * deltaTime;
    currentOffset_.y += currentVelocity_.y * deltaTime;

    // 指定した画面内の限界範囲（クランプ範囲）を超えないように制限する
    currentOffset_.x = std::clamp(currentOffset_.x, moveLimitMin_.x, moveLimitMax_.x);
    currentOffset_.y = std::clamp(currentOffset_.y, moveLimitMin_.y, moveLimitMax_.y);

    // --- 機体の傾き（ロール角）の計算 ---
    // 横移動の速度（currentVelocity_.x）に応じて機体を傾ける (右移動なら右傾き)
    // -zで傾くか+zで傾くかは座標系によるが、基本は符号反転
    float targetRoll = -(currentVelocity_.x / maxSpeed_) * maxRollAngle_;
    rollAngle_ = std::lerp(rollAngle_, targetRoll, 10.0f * deltaTime);

    // --- 浮遊感（サイン波）の計算 ---
    hoverTimer_ += deltaTime * hoverFrequency_;
    float hoverY = std::sin(hoverTimer_) * hoverAmplitude_;

    // --- ローカル座標・回転の更新 ---
    transform->SetPosition({currentOffset_.x, currentOffset_.y + hoverY, 0.0f});
    transform->SetRotation({0.0f, 0.0f, rollAngle_});
}
