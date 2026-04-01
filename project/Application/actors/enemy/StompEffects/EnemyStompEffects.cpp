#include "EnemyStompEffects.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "Irufemi.h" 
#include "core/math/geometry/Math.h"
#include <cmath>
#include <algorithm>

#include "Renderer/LineInstanced/LineClass.h"

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

void EnemyStompEffects::Update(float deltaTime) {
    if (!isActive_) return;

    globalTimer_ += deltaTime;
    phaseTimer_ += deltaTime;

    // --- 1. 最初の足元爆発演出 ---
    if (globalTimer_ < params_.explosionDuration) {
        float t = (std::min)(1.0f, globalTimer_ / params_.explosionDuration);
        float easeOut = 1.0f - static_cast<float>(std::pow(1.0f - t, 2));
        float currentScale = Lerp(1.0f, params_.explosionMaxRadius, easeOut);

        explosionTransform_.scale = { currentScale, currentScale, currentScale };
        explosionObj_->SetColor({ 1.0f, 0.4f, 0.0f, Lerp(params_.explosionInitialAlpha, 0.0f, t) });
    }

    // --- 2. リングと噴き上がり爆発のフェーズ管理 ---
    switch (currentPhase_) {
    case Phase::Expanding:
    {
        float t = (std::min)(1.0f, phaseTimer_ / params_.ringExpandDuration);
        float currentRadius = Lerp(1.0f, params_.ringMaxRadius, t);

        ringTransform_.scale = { currentRadius, params_.ringHeight, currentRadius };
        ringObj_->SetColor(params_.ringColorNormal);

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

        // OBB更新
        UpdateFinalExplosionOBB();

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

bool EnemyStompEffects::IsExplosionDamageActive() const {
    if (globalTimer_ >= params_.explosionDuration) return false;
    float t = globalTimer_ / params_.explosionDuration;
    return t < params_.explosionDamageActiveTime;
}

float EnemyStompEffects::GetExplosionRadius() const {
    float t = (std::min)(1.0f, globalTimer_ / params_.explosionDuration);
    float easeOut = 1.0f - static_cast<float>(std::pow(1.0f - t, 2));
    return Lerp(1.0f, params_.explosionMaxRadius, easeOut);
}

void EnemyStompEffects::DrawDebug(Line3DRegion* lineRegion) {
    if (!lineRegion || !isActive_) return;

    // --- 爆発の当たり判定描画（球状） ---
    if (globalTimer_ < params_.explosionDuration) {
        float t = globalTimer_ / params_.explosionDuration;
        float easeOut = 1.0f - static_cast<float>(std::pow(1.0f - t, 2));
        float currentRadius = Lerp(1.0f, params_.explosionMaxRadius, easeOut);

        // ダメージ判定中なら赤、それ以外はオレンジ
        Vector4 color = (t < params_.explosionDamageActiveTime) ? Vector4{ 1.0f, 0.0f, 0.0f, 1.0f } : Vector4{ 1.0f, 0.5f, 0.0f, 1.0f };

        const int segments = 16;
        const float step = (2.0f * 3.14159265f) / segments;

        for (int i = 0; i < segments; ++i) {
            float theta1 = i * step;
            float theta2 = (i + 1) * step;

            // XZ平面
            Vector3 p1 = { basePosition_.x + currentRadius * std::cos(theta1), basePosition_.y, basePosition_.z + currentRadius * std::sin(theta1) };
            Vector3 p2 = { basePosition_.x + currentRadius * std::cos(theta2), basePosition_.y, basePosition_.z + currentRadius * std::sin(theta2) };
            lineRegion->AddInstance(p1, p2, color);

            // XY平面
            Vector3 p3 = { basePosition_.x + currentRadius * std::cos(theta1), basePosition_.y + currentRadius * std::sin(theta1), basePosition_.z };
            Vector3 p4 = { basePosition_.x + currentRadius * std::cos(theta2), basePosition_.y + currentRadius * std::sin(theta2), basePosition_.z };
            lineRegion->AddInstance(p3, p4, color);

            // YZ平面
            Vector3 p5 = { basePosition_.x, basePosition_.y + currentRadius * std::cos(theta1), basePosition_.z + currentRadius * std::sin(theta1) };
            Vector3 p6 = { basePosition_.x, basePosition_.y + currentRadius * std::cos(theta2), basePosition_.z + currentRadius * std::sin(theta2) };
            lineRegion->AddInstance(p5, p6, color);
        }
    }

    // --- 噴き上がり爆発の当たり判定描画 (OBB) ---
    if (currentPhase_ == Phase::FinalExplosion) {
        Vector4 color = { 1.0f, 0.0f, 0.0f, 1.0f }; // 赤色で判定を描画

        // 8つの頂点を計算
        Vector3 v[8];
        const Vector3& c = finalExplosionOBB_.center;
        const Vector3* axis = finalExplosionOBB_.orientations;
        const Vector3& s = finalExplosionOBB_.size;

        for (int i = 0; i < 8; ++i) {
            v[i] = c;
            Vector3 offset = { 0,0,0 };
            // 各軸の寄与を加算 (iの各ビットで符号を決定)
            offset = Math::Add(offset, Math::Multiply((i & 1) ? s.x : -s.x, axis[0]));
            offset = Math::Add(offset, Math::Multiply((i & 2) ? s.y : -s.y, axis[1]));
            offset = Math::Add(offset, Math::Multiply((i & 4) ? s.z : -s.z, axis[2]));
            v[i] = Math::Add(v[i], offset);
        }

        // 12辺を描画
        // X
        lineRegion->AddInstance(v[0], v[1], color);
        lineRegion->AddInstance(v[2], v[3], color);
        lineRegion->AddInstance(v[4], v[5], color);
        lineRegion->AddInstance(v[6], v[7], color);
        // Y
        lineRegion->AddInstance(v[0], v[2], color);
        lineRegion->AddInstance(v[1], v[3], color);
        lineRegion->AddInstance(v[4], v[6], color);
        lineRegion->AddInstance(v[5], v[7], color);
        // Z
        lineRegion->AddInstance(v[0], v[4], color);
        lineRegion->AddInstance(v[1], v[5], color);
        lineRegion->AddInstance(v[2], v[6], color);
        lineRegion->AddInstance(v[3], v[7], color);
    }
}

void EnemyStompEffects::Draw() {
    if (!isActive_) return;
    if (globalTimer_ < params_.explosionDuration) explosionObj_->Draw();
    if (currentPhase_ != Phase::Finished) ringObj_->Draw();
    if (currentPhase_ == Phase::FinalExplosion) finalExplosionObj_->Draw();
}