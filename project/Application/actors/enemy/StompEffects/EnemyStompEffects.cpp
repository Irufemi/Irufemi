#include "EnemyStompEffects.h"
#include "Irufemi.h" // ObjClass の定義が含まれているはず
#include "Player.h"
#include "core/math/geometry/Math.h"
#include <cmath>

void EnemyStompEffects::Initialize(Camera* camera) {
    camera_ = camera;

    // EnemyBeam.cpp と同じ初期化方法
    explosionObj_ = std::make_unique<ObjClass>();
    explosionObj_->Initialize(camera_, "sample/block.obj"); // 爆発用のモデルパスに
    explosionObj_->SetColor({ 1.0f, 0.3f, 0.0f, 0.6f });

    ringObj_ = std::make_unique<ObjClass>();
    ringObj_->Initialize(camera_, "sample/block.obj"); // リング用のモデルパスに
    ringObj_->SetColor({ 1.0f, 1.0f, 1.0f, 0.4f });

    isActive_ = false;
}

void EnemyStompEffects::Fire(const Vector3& position) {
    basePosition_ = position;
    isActive_ = true;
    timer_ = 0.0f;
    hasDealtExplosionDamage_ = false;
    hasDealtRingDamage_ = false;

    explosionTransform_.translate = position;
    explosionTransform_.scale = { 1.0f, 1.0f, 1.0f };
    explosionTransform_.rotate = { 0, 0, 0 };

    ringTransform_.translate = position;
    ringTransform_.translate.y += 0.2f; // 床へのめり込み防止
    ringTransform_.scale = { 1.0f, 0.2f, 1.0f };
    ringTransform_.rotate = { 0, 0, 0 };
}

void EnemyStompEffects::Update(float deltaTime, Player* player) {
    if (!isActive_) return;
    timer_ += deltaTime;

    // --- 1. 爆発の処理 ---
    if (timer_ < explosionDuration_) {
        float t = timer_ / explosionDuration_;
        float currentExplosionScale = Lerp(1.0f, explosionRadius_, std::sqrt(t));
        explosionTransform_.scale = { currentExplosionScale, currentExplosionScale, currentExplosionScale };

        if (!hasDealtExplosionDamage_ && timer_ < 0.1f) {
            // Math.h にある Vector3 用の減算と距離計算
            Vector3 diff = Math::Subtract(player->GetTranslate(), basePosition_);
            if (Math::Length(diff) <= explosionRadius_) {
                // player->ApplyDamage(50); 
                hasDealtExplosionDamage_ = true;
            }
        }
    }

    // --- 2. 衝撃波（リング）の処理 ---
    if (timer_ < ringDuration_) {
        float t = timer_ / ringDuration_;
        float currentRadius = Lerp(1.0f, ringMaxRadius_, t);

        ringTransform_.scale.x = currentRadius;
        ringTransform_.scale.z = currentRadius;

        if (!hasDealtRingDamage_) {
            Vector3 diff = Math::Subtract(player->GetTranslate(), basePosition_);
            float dist = Math::Length(diff);
            if (dist >= (currentRadius - ringThickness_) && dist <= (currentRadius + ringThickness_)) {
                // player->ApplyDamage(10);
                hasDealtRingDamage_ = true;
            }
        }
    } else {
        isActive_ = false;
    }

    // ObjClass のトランスフォーム更新に合わせて修正
    explosionObj_->SetTransform(explosionTransform_);
    explosionObj_->Update();

    ringObj_->SetTransform(ringTransform_);
    ringObj_->Update();
}

void EnemyStompEffects::Draw() {
    if (!isActive_) return;

    if (timer_ < explosionDuration_) {
        explosionObj_->Draw();
    }
    if (timer_ < ringDuration_) {
        ringObj_->Draw();
    }
}