#define NOMINMAX

#include "CameraController.h"

#include "function/Math.h"
#include "function/Ease.h"
#include "contents/player/Player.h"

#include <algorithm>


// 初期化
void CameraController::Initialize() {

    // カメラの初期化
    camera_.Initialize();
}

// 更新
void CameraController::Update(Camera& camera) {
    // 追従対象のワールドトランスフォームを参照
    const Transform& targetWorldtransform = target_->GetTransform();
    // 追従対象とオフセットと追従対象の速度からカメラの目標座標を計算
    targetPoint_ = Math::Add(Math::Add(targetWorldtransform.translate, targetOffset_), Math::Multiply(kVelocityBias, target_->GetVelocity()));

    // 座標補間によりゆったり追従
    camera_.SetTranslate(Lerp(camera_.GetTranslate(), targetPoint_, kInterpolationrate));

    // 追従対象が画面外に出られないように補正
    {
        float x = std::clamp(camera_.GetTranslate().x, target_->GetTranslate().x + margin.left, target_->GetTranslate().x + margin.right);
        float y = std::clamp(camera_.GetTranslate().y, target_->GetTranslate().y + margin.bottom, target_->GetTranslate().y + margin.top);
        camera_.SetTranslate(Vector3{ x , y ,camera_.GetTranslate().z });
    }


    // 移動範囲制限
    {
        float x = std::clamp(camera_.GetTranslate().x, movableArea_.left, movableArea_.right);
        float y = std::clamp(camera_.GetTranslate().y, movableArea_.bottom, movableArea_.top);
        camera_.SetTranslate(Vector3{ x , y ,camera_.GetTranslate().z });
    }

    // 行列を更新する
    camera.SetTranslate(camera_.GetTranslate());
    camera.SetRotate(camera_.GetRotate());
}

// リセット
void CameraController::Reset() {
    // 追従対象のワールドトランスフォームを参照
    const Transform& targetWorldTransform = target_->GetTransform();
    // 追従対象とオフセットからカメラの座標を計算
    camera_.SetTranslate(Math::Add(targetWorldTransform.translate, targetOffset_));
}