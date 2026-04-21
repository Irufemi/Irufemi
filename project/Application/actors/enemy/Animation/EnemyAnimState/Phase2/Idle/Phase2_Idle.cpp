#include "Phase2_Idle.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include "Engine/Core/Math/Math.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

void Phase2_Idle::Enter(Enemy* enemy) {
    timer_ = 0.0f;
    globalTimer_ = 0.0f;
    wantsToBite_ = false;
    wantsToBeam_ = false;

    orbitSpeed_ = 0.2f + (float)headIndex_ * 0.05f;
    springStrength_ = 0.015f + (float)headIndex_ * 0.005f;
    friction_ = 0.82f;

    wanderTarget_ = {
        (float)(std::rand() % (int)(kFieldLimit * 2) - kFieldLimit),
        kHighHeight,
        (float)(std::rand() % (int)(kFieldLimit * 2) - kFieldLimit)
    };
    
    // 分離時の勢いを一度だけ適用するため、timer_が0の時のみ設定（仮の勢い）
    velocity_ = { (float)(headIndex_ - 1) * 0.3f, 0.4f, (float)(headIndex_ - 1) * 0.3f };
}

void Phase2_Idle::Update(Enemy* enemy, Player* player, float deltaTime) {
    if (!enemy) return;

    timer_ += deltaTime;
    globalTimer_ += deltaTime;

    Transform* headT = nullptr;
    if (headIndex_ == 0) headT = &enemy->GetHeadLeftLocalTransform();
    else if (headIndex_ == 1) headT = &enemy->GetHeadMidLocalTransform();
    else headT = &enemy->GetHeadRightLocalTransform();

    if (!headT) return;

    Vector3 playerPos = (player) ? player->GetTranslate() : Vector3{ 0, 0, 0 };
    Vector3 toPlayer = Math::Subtract(playerPos, headT->translate);
    float distToPlayer = Math::Length(toPlayer);

    // --- 高度の動的制御 ---
    float heightT = (distToPlayer - kHeightChangeDistMin) / (kHeightChangeDistMax - kHeightChangeDistMin);
    heightT = std::clamp(heightT, 0.0f, 1.0f);
    float targetHeight = kLowHeight + heightT * (kHighHeight - kLowHeight);
    
    wanderTarget_.y = targetHeight + std::sin(globalTimer_ * 1.5f + (float)headIndex_) * 1.0f;

    // 目的地更新
    Vector3 diffToTarget = Math::Subtract(wanderTarget_, headT->translate);
    if (Math::Length(diffToTarget) < kWanderArrivalDist) {
        wanderTarget_ = {
            (float)(std::rand() % (int)(kFieldLimit * 2) - kFieldLimit),
            targetHeight,
            (float)(std::rand() % (int)(kFieldLimit * 2) - kFieldLimit)
        };
    }

    // 移動物理
    float currentAccel = springStrength_ * kSpeedMultiplier;
    velocity_ = Math::Add(velocity_, Math::Multiply(currentAccel, diffToTarget));
    velocity_ = Math::Multiply(friction_, velocity_);
    headT->translate = Math::Add(headT->translate, velocity_);

    // 回転
    if (Math::Length(velocity_) > 0.01f) {
        Vector3 moveDir = Math::Normalize(velocity_);
        float targetRotY = std::atan2(moveDir.x, moveDir.z);
        headT->rotate.y += Math::NormalizeAngle(targetRotY - headT->rotate.y) * 0.1f;

        float speed = Math::Length(velocity_);
        headT->rotate.x = speed * 0.3f;
        headT->rotate.z = -std::sin(globalTimer_ * 2.0f + (float)headIndex_) * 0.08f;
    }

    // クールダウン中は攻撃遷移しない
    if (timer_ > cooldownTime_) {
        if (timer_ > kBiteCooldown && distToPlayer < kBiteDistThreshold) {
            wantsToBite_ = true;
        } else if (timer_ > kBeamCooldown && distToPlayer > kBeamDistThreshold) {
            wantsToBeam_ = true;
        }
    }
}

void Phase2_Idle::Exit(Enemy* enemy) {
    wantsToBite_ = false;
    wantsToBeam_ = false;
}
