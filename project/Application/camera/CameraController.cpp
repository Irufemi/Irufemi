#define NOMINMAX

#include "CameraController.h"

#include "function/Math.h"
#include "function/Ease.h"
#include "actors/player/Player.h"

#include <algorithm>
#include <random>


// 初期化
void CameraController::Initialize() {

    // カメラの初期化
    camera_.Initialize();
}

// 更新
void CameraController::Update(Camera& camera) {
    // 追従対象がいなければ何もしない
    if (!target_) {
        return;
    }

    // プレイヤーがダメージを受けたらシェイクを開始
    if (target_->IsJustDamaged()) {
        StartShake(0.3f, 0.2f); // 0.3秒間、振幅0.2で揺らす
    }

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

    // カメラシェイク処理
    if (shakeTimer_ > 0.0f) {
        const float dt = 1.0f / 60.0f; // 60fps前提
        shakeTimer_ -= dt;

        // 乱数生成器
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> distrib(-1.0f, 1.0f);

        // 時間経過で振幅を減衰させる
        float currentAmplitude = shakeAmplitude_ * (shakeTimer_ / 0.3f); // 0.3fはシェイクの持続時間

        // ランダムなオフセットを生成
        Vector3 shakeOffset = {
            distrib(gen) * currentAmplitude,
            distrib(gen) * currentAmplitude,
            0.0f
        };

        // カメラ座標にオフセットを加算
        camera_.SetTranslate(Math::Add(camera_.GetTranslate(), shakeOffset));
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

void CameraController::StartShake(float duration, float amplitude) {
    shakeTimer_ = duration;
    shakeAmplitude_ = amplitude;
}