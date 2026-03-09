#include "EnemyAnimation.h"
#include "Enemy.h"
#include <cmath>

void EnemyAnimation::Initialize(Enemy* enemy) {
    enemy_ = enemy;
}

void EnemyAnimation::Update() {
    if (!enemy_) return;
    timer_ += 1.0f / 60.0f;

    EnemyState state = enemy_->GetState();
    switch (state) {
    case EnemyState::Idle:        UpdateIdle();       break;
    case EnemyState::Attack_Beam: UpdateAttackBeam(); break;
    default: break;
    }
}

void EnemyAnimation::UpdateIdle() {
    attackTimer_ = 0.0f;

    // 1. 自転
    Transform& globalTransform = enemy_->GetGlobalTransform();
    globalTransform.rotate.y += idleRotationSpeed_;

    // 2. 胴体のふわふわ（サイン波）
    for (int i = 0; i < 3; ++i) {
        float targetY = std::sin(timer_ * idleWaveSpeed_ + (float)i * idlePhaseOffset_) * idleWaveHeight_;
        Vector3& offset = enemy_->GetBodyOffset(i);
        offset.y += (targetY - offset.y) * lerpSpeed_;
        offset.x += (0.0f - offset.x) * lerpSpeed_;
    }

    // 3. 頭部のふわふわ（少し位相を遅らせる）
    float headTargetY = std::sin(timer_ * idleWaveSpeed_ + 2.4f) * idleHeadWaveHeight_;
    auto interpolate = [&](Vector3& off) {
        off.y += (headTargetY - off.y) * lerpSpeed_;
        off.x += (0.0f - off.x) * lerpSpeed_;
        };
    interpolate(enemy_->GetHeadLeftOffset());
    interpolate(enemy_->GetHeadMidOffset());
    interpolate(enemy_->GetHeadRightOffset());
}

void EnemyAnimation::UpdateAttackBeam() {
    attackTimer_ += 1.0f / 60.0f;

    if (attackTimer_ < beamGatheringTime_) {
        // --- フェーズ1：溜め（中央集結） ---
        // 左右の頭を寄せる
        enemy_->GetHeadLeftOffset().x += (beamHeadGatherX_ - enemy_->GetHeadLeftOffset().x) * gatherLerp_;
        enemy_->GetHeadRightOffset().x += (-beamHeadGatherX_ - enemy_->GetHeadRightOffset().x) * gatherLerp_;

        // 全体を沈ませる
        enemy_->GetHeadMidOffset().y += (beamSinkDepth_ - enemy_->GetHeadMidOffset().y) * gatherLerp_;
        for (int i = 0; i < 3; ++i) {
            enemy_->GetBodyOffset(i).y += (beamSinkDepth_ * 0.5f - enemy_->GetBodyOffset(i).y) * gatherLerp_;
        }

    } else if (attackTimer_ < (beamGatheringTime_ + beamShakingTime_)) {
        // --- フェーズ2：爆発シェイク ---
        if (!enemy_->IsFiringBeam()) {
            enemy_->FireBeam();
        }

        float shakeX = std::sin(timer_ * beamShakeSpeed_) * beamShakeIntensityX_;
        float shakeY = std::cos(timer_ * (beamShakeSpeed_ + 10.0f)) * beamShakeIntensityY_;

        enemy_->GetHeadMidOffset().x = shakeX;
        enemy_->GetHeadMidOffset().y = beamSinkDepth_ + shakeY;
        enemy_->GetHeadLeftOffset().x = beamHeadGatherX_ + shakeX;
        enemy_->GetHeadRightOffset().x = -beamHeadGatherX_ + shakeX;

        for (int i = 0; i < 3; ++i) {
            enemy_->GetBodyOffset(i).x = shakeX * 0.6f;
            enemy_->GetBodyOffset(i).y = (beamSinkDepth_ * 0.5f) + shakeY;
        }
    } else {
        enemy_->SetState(EnemyState::Idle);
    }
}