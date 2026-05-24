#include "Phase1_Tackle.h"
#include "../../../../Enemy.h"
#include "actors/player/Player.h"
#include "Core/Math/Math.h"
#include "Engine/Graphics/Camera/Camera.h"
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
            if (auto effects = enemy->GetTackleEffects()) {
                effects->StartTelegraph(globalT.translate, globalT.rotate.y, 300.0f, 20.0f);
            }
        }
        break;
    }

    case Phase::Aim: {
        float tAngleY = std::atan2(playerPos.x - globalT.translate.x, playerPos.z - globalT.translate.z);
        globalT.rotate.y += Math::NormalizeAngle(tAngleY - globalT.rotate.y) * 0.2f;

        rushDirection_ = { std::sin(globalT.rotate.y), 0.0f, std::cos(globalT.rotate.y) };

        if (auto effects = enemy->GetTackleEffects()) {
            effects->UpdateTelegraph(globalT.translate, globalT.rotate.y, 0.0f);
        }

        if (stateTimer_ >= kAimTime) {
            currentPhase_ = Phase::Wait;
            stateTimer_ = 0.0f;
        }
        break;
    }

    case Phase::Wait: {
        if (auto effects = enemy->GetTackleEffects()) {
            effects->UpdateTelegraph(globalT.translate, globalT.rotate.y, stateTimer_ / kWaitTime);
        }

        // ロックオン完了後、追尾を止めてタメを作る（回避猶予）
        if (stateTimer_ >= kWaitTime) {
            currentPhase_ = Phase::Rush;
            stateTimer_ = 0.0f;
            rushCount_++;
            if (auto effects = enemy->GetTackleEffects()) {
                effects->StopTelegraph();
            }
        }
        break;
    }

    case Phase::Rush: {
        globalT.translate.x += rushDirection_.x * kRushSpeed;
        globalT.translate.z += rushDirection_.z * kRushSpeed;
        
        // ★新タックル波エフェクトを発生（0.05秒ごと確実に）
        effectTimer_ += deltaTime;
        if (effectTimer_ >= 0.05f) {
             enemy->FireTackleRushWave(globalT.translate);
             effectTimer_ = 0.0f;
        }

        // 壁にぶつかったか判定（1・2回目でも壁外に行かないようにする）
        bool hitWall = false;
        if (std::abs(globalT.translate.x) >= kWallLimit || std::abs(globalT.translate.z) >= kWallLimit) {
            hitWall = true;
            // 壁の外に出ないように位置を制限
            globalT.translate.x = std::clamp(globalT.translate.x, -kWallLimit, kWallLimit);
            globalT.translate.z = std::clamp(globalT.translate.z, -kWallLimit, kWallLimit);
        }

        // 3回目のタックルで壁にドン！した時のみスタン
        if (hitWall && rushCount_ >= kMaxRushCount) {
             currentPhase_ = Phase::Stun;
             stateTimer_ = 0.0f;
             
             // ★壁ドン激突の大爆発エフェクトを一度だけ発生
             enemy->FireTackleCrashWave(globalT.translate);

        } 
        // 1，2回目のタックルで「時間切れ」または「壁に到達」したら、次に移行
        else if (stateTimer_ >= kRushTime || hitWall) {
             if (rushCount_ < kMaxRushCount) {
                 currentPhase_ = Phase::Aim;
                 stateTimer_ = 0.0f;
                 if (auto effects = enemy->GetTackleEffects()) {
                     effects->StartTelegraph(globalT.translate, globalT.rotate.y, 300.0f, 20.0f);
                 }
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
            currentPhase_ = Phase::ReturnToIdle;
            stateTimer_ = 0.0f;
        }
        break;
    }

    case Phase::ReturnToIdle: {
        globalT.scale.y += (kNormalScale - globalT.scale.y) * 0.05f;
        globalT.rotate.x += (0.0f - globalT.rotate.x) * 0.05f;

        auto easeToZero = [](Vector3& v) {
            v.x += (0.0f - v.x) * 0.1f;
            v.y += (0.0f - v.y) * 0.1f;
            v.z += (0.0f - v.z) * 0.1f;
        };

        for (int i = 0; i < 3; ++i) {
            easeToZero(enemy->GetBodyOffset(i));
        }
        easeToZero(enemy->GetHeadLeftOffset());
        easeToZero(enemy->GetHeadMidOffset());
        easeToZero(enemy->GetHeadRightOffset());

        if (stateTimer_ >= kReturnTime) {
            hasFinished_ = true;
        }
        break;
    }
    }
}

void Phase1_Tackle::Exit(Enemy* enemy) {
    if (!enemy) return;
    if (auto effects = enemy->GetTackleEffects()) {
        effects->StopTelegraph();
    }
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
