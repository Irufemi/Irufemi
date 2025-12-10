#include "ui/EnemyHpGauge.h"

#include <algorithm>
#include <cmath>

void EnemyHpGauge::Initialize(Camera* camera, Enemy* enemy) {
    camera_ = camera;
    enemy_ = enemy;

    if (!camera_ || !enemy_) {
        return;
    }

    // 画面サイズからゲージ位置を決める（画面上部中央）
    float screenW = static_cast<float>(camera_->GetViewportWidth());
    // float screenH = static_cast<float>(camera_->GetViewportHeight());

    basePos_.x = screenW * 0.5f-400.0f; // ど真ん中
    basePos_.y = 40.0f;          // 上から少し下

    // テクスチャ読み込み
    frameSprite_.Initialize(camera_, "resources/hp_bar.png");
    fillSprite_.Initialize(camera_, "resources/hp_gauge.png");

    prevPhase_ = enemy_->GetPhase();
    displayedRatio_ = CalcHpRatio();
    lastHp_ = enemy_->GetHp();

    UpdateSpriteTransform();
}

void EnemyHpGauge::Update(float deltaTime) {
    if (!enemy_ || !camera_) {
        return;
    }

    // 敵が完全に倒れたら描画も更新もやめる
    if (enemy_->IsDead()) {
        return;
    }

    // フェーズ遷移検出
    EnemyPhase currentPhase = enemy_->GetPhase();
    if (prevPhase_ == EnemyPhase::Phase1 && currentPhase == EnemyPhase::Phase2) {
        StartPhase2Heal();
    }
    prevPhase_ = currentPhase;

    // HP 減少検出（回復アニメ中は無視）
    if (!healPlaying_) {
        StartShakeOnDamage();
    }

    if (healPlaying_) {
        UpdateHeal(deltaTime);
    } else {
        displayedRatio_ = CalcHpRatio();
    }

    UpdateShake(deltaTime);
    UpdateSpriteTransform();
}

void EnemyHpGauge::Draw() {
    if (!enemy_ || !camera_) {
        return;
    }
    if (enemy_->IsDead()) {
        return;
    }

    frameSprite_.Draw();
    fillSprite_.Draw();
}

// HP / MaxHP の割合を計算
float EnemyHpGauge::CalcHpRatio() const {
    if (!enemy_) {
        return 0.0f;
    }
    int hp = enemy_->GetHp();
    int maxHp = enemy_->GetMaxHp();
    if (maxHp <= 0) {
        return 0.0f;
    }

    float r = static_cast<float>(hp) / static_cast<float>(maxHp);
    return std::clamp(r, 0.0f, 1.0f);
}

// フェーズ2への突入時に回復アニメーション開始
void EnemyHpGauge::StartPhase2Heal() {
    healPlaying_ = true;
    healTimer_ = 0.0f;
    healStartRatio_ = displayedRatio_;
    healEndRatio_ = 1.0f;
}

// HP 減少を検出してシェイク開始
void EnemyHpGauge::StartShakeOnDamage() {
    if (!enemy_) {
        return;
    }

    int currentHp = enemy_->GetHp();

    // 初回だけ基準値として設定
    if (lastHp_ < 0) {
        lastHp_ = currentHp;
        return;
    }

    if (currentHp < lastHp_) {
        shakePlaying_ = true;
        shakeTimer_ = 0.0f;
    }

    lastHp_ = currentHp;
}

// 回復アニメーション更新
void EnemyHpGauge::UpdateHeal(float deltaTime) {
    healTimer_ += deltaTime;
    float t = (healDuration_ > 0.0f) ? (healTimer_ / healDuration_) : 1.0f;
    if (t >= 1.0f) {
        t = 1.0f;
        healPlaying_ = false;
    }

    float e = EaseOutCubic(t);
    displayedRatio_ = healStartRatio_ + (healEndRatio_ - healStartRatio_) * e;
}

// シェイク更新（幅スケールだけ更新）
void EnemyHpGauge::UpdateShake(float deltaTime) {
    if (!shakePlaying_) {
        shakeScale_ = 1.0f;
        return;
    }

    shakeTimer_ += deltaTime;
    if (shakeTimer_ >= shakeDuration_) {
        shakePlaying_ = false;
        shakeScale_ = 1.0f;
        return;
    }

    // 進行度 0→1
    float progress = shakeTimer_ / shakeDuration_;
    // 終盤で揺れが弱くなるようにする
    float strength = (1.0f - progress);

    float phase = shakeTimer_ * shakeFrequency_;
    float offset = std::sin(phase) * shakeMagnitude_ * strength;

    // 1.0 ± offset（offset は ±0.15 くらい）
    shakeScale_ = 1.0f + offset;
}

// スプライトの座標とサイズを更新
void EnemyHpGauge::UpdateSpriteTransform() {
    // === 1. 枠（frameSprite）は常に画面中央固定 ===
    Vector2 framePos = {
        basePos_.x,
        basePos_.y
    };

    frameSprite_.SetPosition(framePos.x, framePos.y);
    frameSprite_.SetSize(barWidth_, barHeight_);
    frameSprite_.Update();

    // === 2. 赤ゲージ（fillSprite）のサイズ計算 ===
    float innerWidth = (barWidth_ - paddingX_ * 2.0f) * displayedRatio_;
    if (innerWidth < 0.0f) innerWidth = 0.0f;

    innerWidth *= shakeScale_;

    float innerHeight = barHeight_ - paddingY_ * 2.0f;
    if (innerHeight < 0.0f) innerHeight = 0.0f;

    // ★★★ ここが補正ポイント（バーカラーを少し右下へ寄せる）★★★
    float offsetX = +4.0f;
    float offsetY = +3.0f;

    // ★ 赤ゲージは枠と同じ中心だがオフセットを加える
    fillSprite_.SetPosition(framePos.x + offsetX, framePos.y + offsetY);

    fillSprite_.SetSize(innerWidth, innerHeight);
    fillSprite_.Update();
}


// イージング関数（0→1 をなめらかに）
float EnemyHpGauge::EaseOutCubic(float t) {
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}
