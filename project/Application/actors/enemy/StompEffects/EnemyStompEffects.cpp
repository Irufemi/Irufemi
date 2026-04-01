#include "EnemyStompEffects.h"
#include "Irufemi.h" 
#include "Player.h"
#include "core/math/geometry/Math.h"
#include <cmath>
#include <algorithm>

void EnemyStompEffects::Initialize(Camera* camera) {
    camera_ = camera;

    explosionObj_ = std::make_unique<ObjClass>();
    explosionObj_->Initialize(camera_, "sample/block.obj");

    ringObj_ = std::make_unique<ObjClass>();
    ringObj_->Initialize(camera_, "sample/block.obj");

    finalExplosionObj_ = std::make_unique<ObjClass>();
    finalExplosionObj_->Initialize(camera_, "sample/block.obj");

    isActive_ = false;
}

void EnemyStompEffects::Fire(const Vector3& position) {
    basePosition_ = position;
    isActive_ = true;
    globalTimer_ = 0.0f;
    phaseTimer_ = 0.0f;
    currentPhase_ = Phase::Expanding;

    hasDealtExplosionDamage_ = false;
    hasDealtRingDamage_ = false;
    hasDealtFinalDamage_ = false;

    // 初期位置設定
    explosionTransform_.translate = position;
    ringTransform_.translate = position;
    ringTransform_.translate.y = basePosition_.y + params_.ringGroundOffset;

    finalExplosionTransform_.translate = position;
    finalExplosionTransform_.translate.y = basePosition_.y + params_.ringGroundOffset;

    // 回転リセット
    explosionTransform_.rotate = { 0,0,0 };
    ringTransform_.rotate = { 0,0,0 };
    finalExplosionTransform_.rotate = { 0,0,0 };
}

void EnemyStompEffects::Update(float deltaTime, Player* player) {
    if (!isActive_ || !player) return;

    globalTimer_ += deltaTime;
    phaseTimer_ += deltaTime;

    // --- 1. 最初の足元爆発演出 ---
    if (globalTimer_ < params_.explosionDuration) {
        float t = (std::min)(1.0f, globalTimer_ / params_.explosionDuration);
        float easeOut = 1.0f - static_cast<float>(std::pow(1.0f - t, 2));
        float currentScale = Lerp(1.0f, params_.explosionMaxRadius, easeOut);

        explosionTransform_.scale = { currentScale, currentScale, currentScale };
        explosionObj_->SetColor({ 1.0f, 0.4f, 0.0f, Lerp(params_.explosionInitialAlpha, 0.0f, t) });

        if (!hasDealtExplosionDamage_ && t < params_.explosionDamageActiveTime) {
            Vector3 diff = Math::Subtract(player->GetTranslate(), basePosition_);
            if (Math::Length(diff) <= currentScale) {
                // player->ApplyDamage(30);
                hasDealtExplosionDamage_ = true;
            }
        }
    }

    // --- 2. リングと噴き上がり爆発のフェーズ管理 ---
    switch (currentPhase_) {
    case Phase::Expanding:
    {
        float t = (std::min)(1.0f, phaseTimer_ / params_.ringExpandDuration);
        float currentRadius = Lerp(1.0f, params_.ringMaxRadius, t);

        ringTransform_.scale = { currentRadius, params_.ringHeight, currentRadius };
        ringObj_->SetColor(params_.ringColorNormal);

        if (!hasDealtRingDamage_) {
            Vector3 diff = Math::Subtract(player->GetTranslate(), basePosition_);
            float dist = Math::Length(diff);
            if (dist >= (currentRadius - params_.ringThickness) && dist <= (currentRadius + params_.ringThickness)) {
                // player->ApplyDamage(10);
                hasDealtRingDamage_ = true;
            }
        }

        if (t >= 1.0f) {
            currentPhase_ = Phase::KeepAndWarning;
            phaseTimer_ = 0.0f;
        }
    }
    break;

    case Phase::KeepAndWarning:
    {
        float t = (std::min)(1.0f, phaseTimer_ / params_.ringKeepDuration);

        // 最大サイズで維持
        ringTransform_.scale = { params_.ringMaxRadius, params_.ringHeight, params_.ringMaxRadius };

        // 徐々に赤らむ予兆演出
        Vector4 c;
        c.x = Lerp(params_.ringColorNormal.x, params_.ringColorWarning.x, t);
        c.y = Lerp(params_.ringColorNormal.y, params_.ringColorWarning.y, t);
        c.z = Lerp(params_.ringColorNormal.z, params_.ringColorWarning.z, t);
        c.w = Lerp(params_.ringColorNormal.w, params_.ringColorWarning.w, t);
        ringObj_->SetColor(c);

        if (t >= 1.0f) {
            currentPhase_ = Phase::FinalExplosion;
            phaseTimer_ = 0.0f;
        }
    }
    break;

    case Phase::FinalExplosion:
    {
        float t = (std::min)(1.0f, phaseTimer_ / params_.finalExplosionDuration);

        // 下から上へ噴き出すアニメーション
        float easeOutH = 1.0f - static_cast<float>(std::pow(1.0f - t, 3));
        float currentH = Lerp(0.01f, params_.finalExplosionMaxHeight, easeOutH);
        float currentR = Lerp(params_.ringMaxRadius, params_.finalExplosionMaxRadius, t);

        finalExplosionTransform_.scale = { currentR, currentH, currentR };

        // ※ピボットがモデルの中心にある場合、座標を上にずらす
        finalExplosionTransform_.translate.y = (basePosition_.y + params_.ringGroundOffset) + (currentH * 0.5f);

        finalExplosionObj_->SetColor({ 1.0f, 0.5f, 0.2f, Lerp(1.0f, 0.0f, t) });

        // OBB更新と判定
        UpdateFinalExplosionOBB();
        if (!hasDealtFinalDamage_) {
            if (CheckOBBCollision(finalExplosionOBB_, player->GetTranslate())) {
                // player->ApplyDamage(params_.finalExplosionDamage);
                hasDealtFinalDamage_ = true;
            }
        }

        if (t >= 1.0f) {
            currentPhase_ = Phase::Finished;
            isActive_ = false;
        }
    }
    break;
    }

    // 行列更新
    if (explosionObj_) {
        explosionObj_->SetTransform(explosionTransform_);
        explosionObj_->Update();
    }
    if (ringObj_) {
        ringObj_->SetTransform(ringTransform_);
        ringObj_->Update();
    }
    if (finalExplosionObj_) {
        finalExplosionObj_->SetTransform(finalExplosionTransform_);
        finalExplosionObj_->Update();
    }
}

void EnemyStompEffects::UpdateFinalExplosionOBB() {
    // 噴き上がっているモデルのTransformからOBBを計算
    finalExplosionOBB_.center = finalExplosionTransform_.translate;

    // 方向（回転がないので基本軸）
    finalExplosionOBB_.orientations[0] = { 1.0f, 0.0f, 0.0f };
    finalExplosionOBB_.orientations[1] = { 0.0f, 1.0f, 0.0f };
    finalExplosionOBB_.orientations[2] = { 0.0f, 0.0f, 1.0f };

    // ハーフサイズをスケールから取得
    finalExplosionOBB_.size.x = finalExplosionTransform_.scale.x * 0.5f;
    finalExplosionOBB_.size.y = finalExplosionTransform_.scale.y * 0.5f;
    finalExplosionOBB_.size.z = finalExplosionTransform_.scale.z * 0.5f;
}

bool EnemyStompEffects::CheckOBBCollision(const OBB& obb, const Vector3& point) {
    Vector3 d = Math::Subtract(point, obb.center);
    for (int i = 0; i < 3; ++i) {
        float distance = std::abs(Math::Dot(d, obb.orientations[i]));
        if (distance > obb.size.x && i == 0) return false;
        if (distance > obb.size.y && i == 1) return false;
        if (distance > obb.size.z && i == 2) return false;
    }
    return true;
}

void EnemyStompEffects::Draw() {
    if (!isActive_) return;
    if (globalTimer_ < params_.explosionDuration) explosionObj_->Draw();
    if (currentPhase_ != Phase::Finished) ringObj_->Draw();
    if (currentPhase_ == Phase::FinalExplosion) finalExplosionObj_->Draw();
}