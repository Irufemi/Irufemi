#include "EnemyStompEffects.h"
#include "Irufemi.h" 
#include "Player.h"
#include "core/math/geometry/Math.h"
#include <cmath>
#include <algorithm>

void EnemyStompEffects::Initialize(Camera* camera) {
    camera_ = camera;

    // --- 爆発エフェクトの準備 ---
    explosionObj_ = std::make_unique<ObjClass>();
    explosionObj_->Initialize(camera_, "sample/block.obj");
    
    // --- 衝撃波リングの準備 ---
    ringObj_ = std::make_unique<ObjClass>();
    ringObj_->Initialize(camera_, "sample/block.obj");

    isActive_ = false;
}

void EnemyStompEffects::Fire(const Vector3& position) {
    // 発生場所を保存
    basePosition_ = position;
    isActive_ = true;
    timer_ = 0.0f;
    hasDealtExplosionDamage_ = false;
    hasDealtRingDamage_ = false;

    // 爆発の初期トランスフォーム設定
    explosionTransform_.translate = position;
    explosionTransform_.scale = { 0.1f, 0.1f, 0.1f };
    explosionTransform_.rotate = { 0, 0, 0 };

    // リングの初期トランスフォーム設定
    ringTransform_.translate = position;
    // Y座標はUpdateで毎フレーム offset を含めて再計算するためここでは初期化のみ
    ringTransform_.scale = { 0.1f, 0.1f, 0.1f };
    ringTransform_.rotate = { 0, 0, 0 };
}

void EnemyStompEffects::Update(float deltaTime, Player* player) {
    if (!isActive_ || !player) return;

    timer_ += deltaTime;

    // --- 1. 爆発ロジック ---
    if (timer_ < params_.explosionDuration) {
        // 進捗率を計算 (0.0 ～ 1.0)
        float t = timer_ / params_.explosionDuration;

        // イージング：最初は速く、後半はゆっくり広がる
        float easeOut = 1.0f - std::pow(1.0f - t, 2);
        float currentScale = Lerp(1.0f, params_.explosionMaxRadius, easeOut);

        explosionTransform_.scale = { currentScale * params_.ringScale.x, currentScale * params_.ringScale.y, currentScale * params_.ringScale.z };

        // 爆発の透明度は時間とともに消えていく
        float alpha = Lerp(params_.explosionInitialAlpha, 0.0f, t);
        explosionObj_->SetColor({ 1.0f, 0.4f, 0.0f, alpha });

        // 爆発のダメージ判定
        if (!hasDealtExplosionDamage_ && t < params_.explosionDamageActiveTime) {
            Vector3 diff = Math::Subtract(player->GetTranslate(), basePosition_);
            float dist = Math::Length(diff);
            if (dist <= currentScale) {
                // player->ApplyDamage(30); 
                hasDealtExplosionDamage_ = true;
            }
        }
    }

    // --- 2. 衝撃波リングロジック ---
    if (timer_ < params_.ringDuration) {
        // 進捗率を計算 (0.0 ～ 1.0)
        float t = timer_ / params_.ringDuration;

        // リングは一定の速度で最大半径まで広がる
        float currentRadius = Lerp(1.0f, params_.ringMaxRadius, t);

        // --- サイズ（スケール）の設定 ---
        ringTransform_.scale.x = currentRadius;
        ringTransform_.scale.y = params_.ringHeight; // モデルの厚み
        ringTransform_.scale.z = currentRadius;

        // --- 高さ（座標）の設定 ---
        // Fireした時の座標（足元）にオフセットを加算して高さを調整
        ringTransform_.translate.y = basePosition_.y + params_.ringGroundOffset;

        // --- 透明度の設定 ---
        // 消さなくて良いとのことなので、常に設定されたアルファ値を維持
        ringObj_->SetColor({ 1.0f, 0.0f, 0.0f, params_.ringAlpha });

        // --- 当たり判定（ドーナツ状） ---
        if (!hasDealtRingDamage_) {
            Vector3 diff = Math::Subtract(player->GetTranslate(), basePosition_);
            float dist = Math::Length(diff);

            // 現在のリングの半径の「縁」の部分にプレイヤーがいるか判定
            if (dist >= (currentRadius - params_.ringThickness) &&
                dist <= (currentRadius + params_.ringThickness)) {
                // player->ApplyDamage(10);
                hasDealtRingDamage_ = true;
            }
        }
    }

    // 全てのエフェクトの時間が終了したかチェック
    if (timer_ >= params_.ringDuration && timer_ >= params_.explosionDuration) {
        isActive_ = false;
    }

    // --- オブジェクトの行列更新 ---
    if (explosionObj_) {
        explosionObj_->SetTransform(explosionTransform_);
        explosionObj_->Update();
    }
    if (ringObj_) {
        ringObj_->SetTransform(ringTransform_);
        ringObj_->Update();
    }
}

void EnemyStompEffects::Draw() {
    if (!isActive_) return;

    // 爆発の描画（持続時間内のみ）
    if (timer_ < params_.explosionDuration && explosionObj_) {
        explosionObj_->Draw();
    }

    // リングの描画（持続時間内のみ）
    if (timer_ < params_.ringDuration && ringObj_) {
        ringObj_->Draw();
    }
}