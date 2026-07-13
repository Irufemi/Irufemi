#include "RailShooterPlayerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <algorithm>
#include <cmath>

void RailShooterPlayerComponent::OnRegisterProperties() {
    RegisterProperty("XYSpeed", &xySpeed_);
    RegisterProperty("MoveLimitMin", &moveLimitMin_);
    RegisterProperty("MoveLimitMax", &moveLimitMax_);
}

void RailShooterPlayerComponent::Update() {
    if (!gameObject_) return;

    // 1フレームの経過時間
    float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (deltaTime <= 0.0f) {
        return;
    }

    auto transform = gameObject_->GetComponent<TransformComponent>();
    if (!transform) return;

    // --- キー入力による上下左右の回避運動 ---
    auto* input = BaseModel::GetIrufemiEngine()->GetInputManager();
    Vector3 moveDir = {0.0f, 0.0f, 0.0f};

    // WASD または 矢印キーで移動方向を入力
    if (input->IsKeyPressedDIK(DIK_W) || input->IsKeyPressedDIK(DIK_UP)) moveDir.y += 1.0f;
    if (input->IsKeyPressedDIK(DIK_S) || input->IsKeyPressedDIK(DIK_DOWN)) moveDir.y -= 1.0f;
    if (input->IsKeyPressedDIK(DIK_A) || input->IsKeyPressedDIK(DIK_LEFT)) moveDir.x -= 1.0f;
    if (input->IsKeyPressedDIK(DIK_D) || input->IsKeyPressedDIK(DIK_RIGHT)) moveDir.x += 1.0f;

    // 斜め移動したときに移動速度が速くならないように、ベクトルの長さを1に抑える
    if (moveDir.x != 0.0f || moveDir.y != 0.0f) {
        float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        moveDir.x /= len;
        moveDir.y /= len;

        // レール中心からのズレ幅（オフセット値）を増やす
        currentOffset_.x += moveDir.x * xySpeed_ * deltaTime;
        currentOffset_.y += moveDir.y * xySpeed_ * deltaTime;

        // 指定した画面内の限界範囲（クランプ範囲）を超えないように制限する
        currentOffset_.x = std::clamp(currentOffset_.x, moveLimitMin_.x, moveLimitMax_.x);
        currentOffset_.y = std::clamp(currentOffset_.y, moveLimitMin_.y, moveLimitMax_.y);
    }

    // --- ローカル座標の更新 ---
    // 親オブジェクト（SplineFollowerComponent）がレール上の位置と回転（進行方向）を決定しているため、
    // 子であるこのコンポーネントはローカル座標のXYを変更するだけで、自動的に「レールから見た上下左右」に移動します。
    transform->SetPosition({currentOffset_.x, currentOffset_.y, 0.0f});
}
