#include "EnemyAnimState_Beam.h"
#include "Enemy.h"
#include "Beam/EnemyBeam.h"
#include "actors/player/Player.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void EnemyAnimState_Beam::Enter(Enemy* enemy) {
    attackTimer_ = 0.0f;
    isLockedOn_ = false;
    isFiring_ = false;
    hasFinishedAttack_ = false; // これが重要
    // totalTimer_ は継続させてOK
}

void EnemyAnimState_Beam::Update(Enemy* enemy, Player* player, float deltaTime) {
    attackTimer_ += deltaTime;
    totalTimer_ += deltaTime; // 振動計算用の timer_ 代わり

    EnemyBeam* beam = enemy->GetBeam();
    Matrix4x4 headMatrix = enemy->GetHeadMidWorldMatrix();
    // 元の headExtensionY_ を使用した座標計算
    Vector3 headPos = { headMatrix.m[3][0], headMatrix.m[3][1] + headExtensionY_, headMatrix.m[3][2] };

    float endCharge = chargeTime_;
    float endAnticipation = endCharge + anticipationTime_;
    float endFire = endAnticipation + fireTime_;
    float endStun = endFire + stunTime_;
    float endRecovery = endStun + recoveryTime_;

    // 1. チャージ（追尾 ＆ 各部バラバラの震え ＆ 徐々に前傾）
    if (attackTimer_ < endCharge) {
        float sp = shakeBaseSpeed_;
        enemy->GetGlobalTransform().rotate.x += (fireLeanAngleX_ - enemy->GetGlobalTransform().rotate.x) * 0.05f;

        auto SetShake = [&](Vector3& offset, float seed) {
            offset = { std::sin(totalTimer_ * sp * seed) * chargeHeadShake_, std::cos(totalTimer_ * sp * (seed + 0.1f)) * chargeHeadShake_, 0 };
            };
        SetShake(enemy->GetHeadMidOffset(), 1.0f);
        SetShake(enemy->GetHeadLeftOffset(), 1.2f);
        SetShake(enemy->GetHeadRightOffset(), 0.8f);

        for (int i = 0; i < 3; ++i) {
            enemy->GetBodyOffset(i).x = std::sin(totalTimer_ * sp * 0.7f + (float)i) * chargeBodyShake_;
        }

        Vector3 target = (player) ? player->GetTranslate() : Vector3{ 0,0,0 };
        target.y += 1.0f;
        float tAngleY = std::atan2(target.x - enemy->GetGlobalTransform().translate.x, target.z - enemy->GetGlobalTransform().translate.z);
        enemy->GetGlobalTransform().rotate.y += NormalizeAngle(tAngleY - enemy->GetGlobalTransform().rotate.y) * beamRotateSpeed_;

        enemy->FireBeam(); // ここでビーム生成をトリガー
        if (beam) {
            beam->SetActive(true);
            beam->SetThickness(0.2f);
            beam->Update(headPos, target);
        }
    }
    // 2. 溜め
    else if (attackTimer_ < endAnticipation) {
        if (!isLockedOn_ && player) {
            lockedTargetPos_ = player->GetTranslate();
            lockedTargetPos_.y += 1.0f;
            isLockedOn_ = true;
        }
        enemy->GetHeadMidOffset() = { 0,0,0 };
        enemy->GetHeadLeftOffset() = { 0,0,0 };
        enemy->GetHeadRightOffset() = { 0,0,0 };
        for (int i = 0; i < 3; ++i) enemy->GetBodyOffset(i).x *= 0.5f;

        if (beam) beam->Update(headPos, lockedTargetPos_);
    }
    // 3. 本射（激しい全身振動 ＆ 前傾維持）
    else if (attackTimer_ < endFire) {
        isFiring_ = true;
        float fireProgress = (attackTimer_ - endAnticipation) / fireTime_;
        float sp = shakeBaseSpeed_ * 1.3f;

        auto SetFireShake = [&](Vector3& offset, float seed) {
            offset = { std::sin(totalTimer_ * sp * seed) * fireHeadShake_, std::cos(totalTimer_ * sp * (seed + 0.2f)) * fireHeadShake_, 0 };
            };
        SetFireShake(enemy->GetHeadMidOffset(), 1.1f);
        SetFireShake(enemy->GetHeadLeftOffset(), 0.95f);
        SetFireShake(enemy->GetHeadRightOffset(), 1.15f);

        for (int i = 0; i < 3; ++i) {
            enemy->GetBodyOffset(i).x = std::sin(totalTimer_ * sp * 0.8f + (float)i) * fireBodyShake_;
            enemy->GetBodyOffset(i).y += (std::cos(totalTimer_ * sp * 0.5f) * 0.1f - enemy->GetBodyOffset(i).y) * 0.1f;
        }

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
    // 4. 硬直
    else if (attackTimer_ < endStun) {
        isFiring_ = false;
        if (beam) beam->SetActive(false);
        enemy->GetGlobalTransform().rotate.x += (0.0f - enemy->GetGlobalTransform().rotate.x) * returnSpeed_;

        float sp = shakeBaseSpeed_ * 0.5f;
        auto SetStunPos = [&](Vector3& offset, float seed) {
            offset.x = std::sin(totalTimer_ * sp * seed) * stunShakeStrength_;
            offset.z += (-1.0f - offset.z) * 0.15f;
            };
        SetStunPos(enemy->GetHeadMidOffset(), 1.0f);
        SetStunPos(enemy->GetHeadLeftOffset(), 1.1f);
        SetStunPos(enemy->GetHeadRightOffset(), 0.9f);
    }
    // 5. 一呼吸
    else if (attackTimer_ < endRecovery) {
        float recProgress = (attackTimer_ - endStun) / recoveryTime_;
        float breathCurve = std::sin(recProgress * (float)M_PI);
        float currentExhaustion = (1.0f - recProgress) * exhaustionDepth_ - (breathCurve * 0.5f);

        auto ApplyExhaustion = [&](Vector3& offset) {
            offset.y += (currentExhaustion - offset.y) * lerpSpeed_;
            offset.z += (0.0f - offset.z) * returnSpeed_;
            };
        ApplyExhaustion(enemy->GetHeadMidOffset());
        ApplyExhaustion(enemy->GetHeadLeftOffset());
        ApplyExhaustion(enemy->GetHeadRightOffset());

        for (int i = 0; i < 3; ++i) {
            enemy->GetBodyOffset(i).y += (currentExhaustion * 0.7f - enemy->GetBodyOffset(i).y) * lerpSpeed_;
        }
    } else {
        hasFinishedAttack_ = true;
        enemy->SetState(EnemyState::Idle);
    }
}

void EnemyAnimState_Beam::Exit(Enemy* enemy) {
    if (auto* beam = enemy->GetBeam()) beam->SetActive(false);
}

float EnemyAnimState_Beam::NormalizeAngle(float angle) {
    while (angle > (float)M_PI) angle -= 2.0f * (float)M_PI;
    while (angle < -(float)M_PI) angle += 2.0f * (float)M_PI;
    return angle;
}