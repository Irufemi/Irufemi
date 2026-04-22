#include "Phase1_Tackle.h"
#include "../../../../Enemy.h"
#include "actors/player/Player.h"
#include "Core/Math/Math.h"
#include "camera/Camera.h"
#include <cmath>
#include <algorithm>

void Phase1_Tackle::Enter(Enemy* enemy) {
    currentPhase_ = Phase::PreAttack;
    stateTimer_ = 0.0f;
    totalTimer_ = 0.0f;
    hasFinished_ = false;
    rushCount_ = 0;
    rushDirection_ = { 0, 0, 0 };
    if (enemy) {
        targetRotateY_ = enemy->GetGlobalTransform().rotate.y;
    }
}

void Phase1_Tackle::Update(Enemy* enemy, Player* player, float deltaTime) {
    if (!enemy) return;

    stateTimer_ += deltaTime;
    totalTimer_ += deltaTime;

    Transform& globalT = enemy->GetGlobalTransform();
    Vector3 playerPos = player ? player->GetTranslate() : Vector3{ 0,0,0 };

    switch (currentPhase_) {
    case Phase::PreAttack: {
        globalT.scale.y += (kSquashScale - globalT.scale.y) * 0.1f;
        
        for (int i = 0; i < 3; ++i) {
            enemy->GetBodyOffset(i).x = std::sin(totalTimer_ * 50.0f + i) * kShakeIntensity;
            enemy->GetBodyOffset(i).z = std::cos(totalTimer_ * 40.0f + i) * kShakeIntensity;
        }

        enemy->GetHeadMidOffset().z += (kNeckExtension - enemy->GetHeadMidOffset().z) * 0.1f;
        enemy->GetHeadLeftOffset().z += (kNeckExtension - enemy->GetHeadLeftOffset().z) * 0.1f;
        enemy->GetHeadRightOffset().z += (kNeckExtension - enemy->GetHeadRightOffset().z) * 0.1f;

        if (stateTimer_ >= kPreAttackTime) {
            currentPhase_ = Phase::Aim;
            stateTimer_ = 0.0f;
            
            for (int i = 0; i < 3; ++i) {
                enemy->GetBodyOffset(i) = { 0,0,0 };
            }
        }
        break;
    }

    case Phase::Aim: {
        float tAngleY = std::atan2(playerPos.x - globalT.translate.x, playerPos.z - globalT.translate.z);
        globalT.rotate.y += Math::NormalizeAngle(tAngleY - globalT.rotate.y) * 0.2f;

        rushDirection_ = { std::sin(globalT.rotate.y), 0.0f, std::cos(globalT.rotate.y) };

        if (stateTimer_ >= kAimTime) {
            currentPhase_ = Phase::Rush;
            stateTimer_ = 0.0f;
            rushCount_++;
        }
        break;
    }

    case Phase::Rush: {
        globalT.translate.x += rushDirection_.x * kRushSpeed;
        globalT.translate.z += rushDirection_.z * kRushSpeed;
        
        // ★新タックル波エフェクトを発生（0.1秒ごとなど細かく）
        if (std::fmod(totalTimer_, 0.05f) < deltaTime) {
             enemy->FireTackleRushWave(globalT.translate);
        }

        bool hitWall = false;
        if (rushCount_ == kMaxRushCount) {
            if (std::abs(globalT.translate.x) >= kWallLimit || std::abs(globalT.translate.z) >= kWallLimit) {
                hitWall = true;
            }
        }

        if (hitWall) {
             currentPhase_ = Phase::Stun;
             stateTimer_ = 0.0f;
             globalT.translate.x = std::clamp(globalT.translate.x, -kWallLimit, kWallLimit);
             globalT.translate.z = std::clamp(globalT.translate.z, -kWallLimit, kWallLimit);
             
             // ★壁ドン激突の大爆発エフェクトを一度だけ発生
             enemy->FireTackleCrashWave(globalT.translate);

        } else if (stateTimer_ >= kRushTime) {
             if (rushCount_ < kMaxRushCount) {
                 currentPhase_ = Phase::Aim;
                 stateTimer_ = 0.0f;
             }
        }
        break;
    }

    case Phase::Stun: {
        globalT.scale.y += (kNormalScale - globalT.scale.y) * 0.05f;

        globalT.rotate.x += (0.5f - globalT.rotate.x) * 0.1f;
        
        enemy->GetHeadMidOffset().x = std::sin(totalTimer_ * 5.0f) * 0.2f;
        enemy->GetHeadLeftOffset().x = std::sin(totalTimer_ * 5.5f) * 0.2f;
        enemy->GetHeadRightOffset().x = std::sin(totalTimer_ * 4.5f) * 0.2f;

        if (stateTimer_ >= kStunTime) {
            hasFinished_ = true;
        }
        break;
    }
    }
}

void Phase1_Tackle::Exit(Enemy* enemy) {
    if (!enemy) return;
    Transform& globalT = enemy->GetGlobalTransform();
    globalT.scale.y = kNormalScale;
    globalT.rotate.x = 0.0f;

    for (int i = 0; i < 3; ++i) {
        enemy->GetBodyOffset(i) = { 0, 0, 0 };
    }
    enemy->GetHeadMidOffset() = { 0, 0, 0 };
    enemy->GetHeadLeftOffset() = { 0, 0, 0 };
    enemy->GetHeadRightOffset() = { 0, 0, 0 };
}
