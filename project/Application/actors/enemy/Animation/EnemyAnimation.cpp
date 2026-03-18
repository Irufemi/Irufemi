#include "EnemyAnimation.h"
#include "Enemy.h"
#include "Player.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void EnemyAnimation::Initialize(Enemy* enemy) {
    enemy_ = enemy;
    hasFinishedAttack_ = false;
}

void EnemyAnimation::Update(Player* player) {
    if (!enemy_) return;
    timer_ += 1.0f / 60.0f;

    EnemyState state = enemy_->GetState();
    switch (state) {
    case EnemyState::Idle:        UpdateIdle(); break;
    case EnemyState::Attack_Beam: UpdateAttackBeam(player); break;
    }
}

// --- 待機状態：通常の呼吸と自転 ---
void EnemyAnimation::UpdateIdle() {
    attackTimer_ = 0.0f;
    isLockedOn_ = false;
    isFiring_ = false;
    hasFinishedAttack_ = false; // Idleに戻っている間はfalse

    float ls = lerpSpeed_;

    // 各首の呼吸（縦揺れ）
    auto ApplyIdleBreath = [&](Vector3& offset, float phase) {
        float waveY = std::sin(timer_ * breathSpeed_ + phase) * breathHeight_;
        offset.x += (0.0f - offset.x) * returnSpeed_;
        offset.y += (waveY - offset.y) * ls;
        offset.z += (0.0f - offset.z) * returnSpeed_;
        };
    ApplyIdleBreath(enemy_->GetHeadMidOffset(), 0.0f);
    ApplyIdleBreath(enemy_->GetHeadLeftOffset(), phaseOffset_);
    ApplyIdleBreath(enemy_->GetHeadRightOffset(), phaseOffset_ * 2.0f);

    // 胴体の波打ち
    for (int i = 0; i < 3; ++i) {
        float waveBodyY = std::sin(timer_ * breathSpeed_ - (float)(i + 1) * phaseOffset_) * bodyWaveHeight_;
        enemy_->GetBodyOffset(i).y += (waveBodyY - enemy_->GetBodyOffset(i).y) * ls;
        enemy_->GetBodyOffset(i).x += (0.0f - enemy_->GetBodyOffset(i).x) * returnSpeed_;
    }

    // 全体の回転を戻す
    enemy_->GetGlobalTransform().rotate.y += idleRotationSpeed_;
    enemy_->GetGlobalTransform().rotate.x += (0.0f - enemy_->GetGlobalTransform().rotate.x) * returnSpeed_;
}

// --- ビーム攻撃：前傾姿勢・個別振動・後隙 ---
void EnemyAnimation::UpdateAttackBeam(Player* player) {
    attackTimer_ += 1.0f / 60.0f;
    EnemyBeam* beam = enemy_->GetBeam();

    Matrix4x4 headMatrix = enemy_->GetHeadMidWorldMatrix();
    Vector3 headPos = { headMatrix.m[3][0], headMatrix.m[3][1] + headExtensionY_, headMatrix.m[3][2] };

    float endCharge = chargeTime_;
    float endAnticipation = endCharge + anticipationTime_;
    float endFire = endAnticipation + fireTime_;
    float endStun = endFire + stunTime_;
    float endRecovery = endStun + recoveryTime_;

    // 1. チャージ（追尾 ＆ 各部バラバラの震え ＆ 徐々に前傾）
    if (attackTimer_ < endCharge) {
        float progress = attackTimer_ / chargeTime_;
        float sp = shakeBaseSpeed_;

        // 前傾姿勢へ移行
        enemy_->GetGlobalTransform().rotate.x += (fireLeanAngleX_ - enemy_->GetGlobalTransform().rotate.x) * 0.05f;

        // 頭部の個別振動
        auto SetShake = [&](Vector3& offset, float seed) {
            offset = { std::sin(timer_ * sp * seed) * chargeHeadShake_, std::cos(timer_ * sp * (seed + 0.1f)) * chargeHeadShake_, 0 };
            };
        SetShake(enemy_->GetHeadMidOffset(), 1.0f);
        SetShake(enemy_->GetHeadLeftOffset(), 1.2f);
        SetShake(enemy_->GetHeadRightOffset(), 0.8f);

        // 体の個別振動
        for (int i = 0; i < 3; ++i) {
            enemy_->GetBodyOffset(i).x = std::sin(timer_ * sp * 0.7f + (float)i) * chargeBodyShake_;
        }

        Vector3 target = (player) ? player->GetTranslate() : Vector3{ 0,0,0 };
        target.y += 1.0f;
        float tAngleY = std::atan2(target.x - enemy_->GetGlobalTransform().translate.x, target.z - enemy_->GetGlobalTransform().translate.z);
        enemy_->GetGlobalTransform().rotate.y += NormalizeAngle(tAngleY - enemy_->GetGlobalTransform().rotate.y) * beamRotateSpeed_;

        enemy_->FireBeam();
        if (beam) {
            beam->SetActive(true);
            beam->SetThickness(0.2f);
            beam->Update(headPos, target);
        }
    }
    // 2. 溜め（一瞬止まって集中）
    else if (attackTimer_ < endAnticipation) {
        if (!isLockedOn_ && player) {
            lockedTargetPos_ = player->GetTranslate(); lockedTargetPos_.y += 1.0f;
            isLockedOn_ = true;
        }
        // 振動を小さくして「溜め」を表現
        enemy_->GetHeadMidOffset() = { 0,0,0 };
        enemy_->GetHeadLeftOffset() = { 0,0,0 };
        enemy_->GetHeadRightOffset() = { 0,0,0 };
        for (int i = 0; i < 3; ++i) enemy_->GetBodyOffset(i).x *= 0.5f;

        if (beam) beam->Update(headPos, lockedTargetPos_);
    }
    // 3. 本射（激しい全身振動 ＆ 前傾維持）
    else if (attackTimer_ < endFire) {
        isFiring_ = true;
        float fireProgress = (attackTimer_ - endAnticipation) / fireTime_;
        float sp = shakeBaseSpeed_ * 1.3f; // 高速振動

        // 全頭部の激しい個別振動
        auto SetFireShake = [&](Vector3& offset, float seed) {
            offset = { std::sin(timer_ * sp * seed) * fireHeadShake_, std::cos(timer_ * sp * (seed + 0.2f)) * fireHeadShake_, 0 };
            };
        SetFireShake(enemy_->GetHeadMidOffset(), 1.1f);
        SetFireShake(enemy_->GetHeadLeftOffset(), 0.95f);
        SetFireShake(enemy_->GetHeadRightOffset(), 1.15f);

        // 体もバチバチに震わせる
        for (int i = 0; i < 3; ++i) {
            enemy_->GetBodyOffset(i).x = std::sin(timer_ * sp * 0.8f + (float)i) * fireBodyShake_;
            enemy_->GetBodyOffset(i).y += (std::cos(timer_ * sp * 0.5f) * 0.1f - enemy_->GetBodyOffset(i).y) * 0.1f;
        }

        // ビーム太さ演出
        if (beam) {
            float thickness = beamThicknessFire_;
            if (fireProgress > fadeOutStartThreshold_) {
                float f = (fireProgress - fadeOutStartThreshold_) / (1.0f - fadeOutStartThreshold_);
                thickness += (std::pow(f, 3.0f) * beamThicknessFire_ * beamExpandScale_);
            }
            beam->SetThickness(thickness);
            beam->Update(headPos, lockedTargetPos_);
        }
    }
    // 4. 硬直（ビーム消滅 ＆ 姿勢をIdleの状態へ戻し始める）
    else if (attackTimer_ < endStun) {
        isFiring_ = false;
        if (beam) beam->SetActive(false);

        // 前傾を戻す（反動でのけぞり気味に戻す）
        enemy_->GetGlobalTransform().rotate.x += (0.0f - enemy_->GetGlobalTransform().rotate.x) * returnSpeed_;

        float sp = shakeBaseSpeed_ * 0.5f;
        auto SetStunPos = [&](Vector3& offset, float seed) {
            offset.x = std::sin(timer_ * sp * seed) * stunShakeStrength_;
            offset.z += (-1.0f - offset.z) * 0.15f; // 反動で少し引く
            };
        SetStunPos(enemy_->GetHeadMidOffset(), 1.0f);
        SetStunPos(enemy_->GetHeadLeftOffset(), 1.1f);
        SetStunPos(enemy_->GetHeadRightOffset(), 0.9f);
    }
    // 5. 一呼吸（ガクッと力を抜く ＆ 完了フラグ）
    else if (attackTimer_ < endRecovery) {
        float recProgress = (attackTimer_ - endStun) / recoveryTime_;
        float breathCurve = std::sin(recProgress * (float)M_PI);
        float currentExhaustion = (1.0f - recProgress) * exhaustionDepth_ - (breathCurve * 0.5f);

        auto ApplyExhaustion = [&](Vector3& offset) {
            offset.y += (currentExhaustion - offset.y) * lerpSpeed_;
            offset.z += (0.0f - offset.z) * returnSpeed_;
            };
        ApplyExhaustion(enemy_->GetHeadMidOffset());
        ApplyExhaustion(enemy_->GetHeadLeftOffset());
        ApplyExhaustion(enemy_->GetHeadRightOffset());

        for (int i = 0; i < 3; ++i) {
            enemy_->GetBodyOffset(i).y += (currentExhaustion * 0.7f - enemy_->GetBodyOffset(i).y) * lerpSpeed_;
        }
    } else {
        // 全行程完了
        hasFinishedAttack_ = true;
        enemy_->SetState(EnemyState::Idle);
    }
}

float EnemyAnimation::NormalizeAngle(float angle) {
    while (angle > (float)M_PI) angle -= 2.0f * (float)M_PI;
    while (angle < -(float)M_PI) angle += 2.0f * (float)M_PI;
    return angle;
}