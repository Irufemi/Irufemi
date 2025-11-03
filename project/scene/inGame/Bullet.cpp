#include "Bullet.h"
#include <cmath>
#include <imgui.h>
#include "camera/Camera.h"
#include "function/Math.h"

#include "ScreenSpace.h"

void Bullet::Initialize(Vector2 pos, float sin, float cos, Camera* camera) {
    pos_ = pos;
    radius_ = { 15.0f, 15.0f };
    velocity_ = { sin * 10.0f, cos * 10.0f }; // 発射方向に速度を設定
    isActive_ = false;
    recoverTime_ = 60;
    t_ = 0.0f;
    result_ = 0.0f;
    isReturn_ = false;

    camera_ = camera;

    sphereRegion_ = std::make_unique<SphereRegion>();
    // テクスチャ名は適宜差し替え
    sphereRegion_->Initialize(camera_, "resources/whiteTexture.png", 16);
    sphereRegion_->SetColor(Vector4{ 0.4f,0.7f,0.7f,1.0f });
}

void Bullet::JudgeScreen() {
    if (pos_.x > 250.0f) {
        area_ = true;
    } else {
        area_ = false;
    }
}

void Bullet::SpeedCalculation() {
    if (pos_.x - radius_.x <= 0.0f || pos_.x + radius_.x >= 500.0f) {
        wallTouch_ = true;
        velocity_.x *= -1.0f;
    }
    if (pos_.y - radius_.y <= 0.0f) {
        velocity_.y = 0.0f;
    }
    velocity_.y += kGravity * deltaTime;
}

void Bullet::Recover() {
    if (recoverTime_ >= 0) {
        recoverTime_--;
    }
}

void Bullet::Collect() {
    if (isActive_) {
        recoverTime_ = 0;
    }
}

float Bullet::moveT(float second) {
    float frame = second * 60;
    float moveSpeed = 1.0f / frame;

    if (t_ <= 1.0f) {
        t_ += moveSpeed;
    }

    return t_;
}

float Bullet::easeInExpo(float t) {
    return (t == 0.0f) ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f);
}

Vector2 Bullet::Return(Vector2 pos, float t) {
    Vector2 v;

    v.x = t * pos.x + (1 - t) * pos_.x;
    v.y = t * pos.y + (1 - t) * pos_.y;

    return v;
}

void Bullet::Update() {
    if (!isActive_) return;

    JudgeScreen();
    SpeedCalculation();

    // 座標更新
    pos_.x += velocity_.x;
    pos_.y -= velocity_.y;

    //壁へのめり込み予防
    if (pos_.x - radius_.x < 0.0f) {
        pos_.x = 0.0f + radius_.x;
    } else if (pos_.x + radius_.x > 500.0f) {
        pos_.x = 500.0f - radius_.x;
    }
}

void Bullet::Draw() {
    if (!isActive_ && !isReturn_) return;

    // スクリーン座標 → Z=0 のワールド座標へ
    const float targetZ = 0.0f;
    Vector3 wc = ScreenToWorldOnZ(camera_, pos_, targetZ);
    float wr = ScreenRadiusToWorld(camera_, pos_, radius_.x, targetZ);

    // 単一インスタンスを積んで描画
    sphereRegion_->ClearInstances();
    sphereRegion_->AddInstance(wc, wr); // 等方スケールの球として追加
    sphereRegion_->Draw();
}