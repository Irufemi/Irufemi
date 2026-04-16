#include "Phase2.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include "Engine/Core/Math/Math.h"
#include "Beam/EnemyBeam.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

void Phase2::Enter(Enemy* enemy) {
    globalTimer_ = 0.0f;
    
    // 首の初期化
    for (int i = 0; i < 3; ++i) {
        headStates_[i].mode = HeadState::Mode::Floating;
        headStates_[i].behaviorTimer = 0.0f;
        
        // 基本性能のバラつき
        headStates_[i].orbitSpeed = 0.2f + (float)i * 0.05f;
        headStates_[i].springStrength = 0.015f + (float)i * 0.005f;
        headStates_[i].friction = kFrictionBase;
        
        // 初期の徘徊目的地をランダムに設定
        headStates_[i].wanderTarget = {
            (float)(std::rand() % (int)(kFieldLimit * 2) - kFieldLimit),
            kHighHeight,
            (float)(std::rand() % (int)(kFieldLimit * 2) - kFieldLimit)
        };
        
        // 分離の勢い
        headStates_[i].velocity = { (float)(i - 1) * 0.3f, 0.4f, (float)(i - 1) * 0.3f };

        // カミツキステートの個別初期化
        biteStates_[i] = std::make_unique<Bite>();
        biteStates_[i]->SetHeadIndex(i);
    }
}

void Phase2::Update(Enemy* enemy, Player* player, float deltaTime) {
    globalTimer_ += deltaTime;

    for (int i = 0; i < 3; ++i) {
        HeadState& state = headStates_[i];
        state.behaviorTimer += deltaTime;

        switch (state.mode) {
        case HeadState::Mode::Floating:
            UpdateFloating(i, state, enemy, player, deltaTime);
            break;
        case HeadState::Mode::Biting:
            UpdateBiting(i, state, enemy, player, deltaTime);
            break;
        case HeadState::Mode::Beaming:
            UpdateBeaming(i, state, enemy, player, deltaTime);
            break;
        case HeadState::Mode::Recovering:
            UpdateFloating(i, state, enemy, player, deltaTime);
            if (state.behaviorTimer > 1.0f) {
                state.mode = HeadState::Mode::Floating;
                state.behaviorTimer = 0.0f;
            }
            break;
        }
    }

    ApplyRepulsion(enemy);
}

void Phase2::Exit(Enemy* enemy) {
}

void Phase2::UpdateFloating(int i, HeadState& state, Enemy* enemy, Player* player, float deltaTime) {
    Transform& headT = (i == 0) ? enemy->GetHeadLeftLocalTransform() :
                       (i == 1) ? enemy->GetHeadMidLocalTransform() :
                                  enemy->GetHeadRightLocalTransform();
    
    Vector3 playerPos = (player) ? player->GetTranslate() : Vector3{ 0, 0, 0 };
    Vector3 toPlayer = Math::Subtract(playerPos, headT.translate);
    float distToPlayer = Math::Length(toPlayer);

    // --- 高度の動的制御 ---
    float heightT = (distToPlayer - kHeightChangeDistMin) / (kHeightChangeDistMax - kHeightChangeDistMin);
    heightT = std::clamp(heightT, 0.0f, 1.0f);
    float targetHeight = kLowHeight + heightT * (kHighHeight - kLowHeight);
    
    state.wanderTarget.y = targetHeight + std::sin(globalTimer_ * 1.5f + (float)i) * 1.0f;

    // 目的地更新
    Vector3 diffToTarget = Math::Subtract(state.wanderTarget, headT.translate);
    if (Math::Length(diffToTarget) < kWanderArrivalDist) {
        state.wanderTarget = {
            (float)(std::rand() % (int)(kFieldLimit * 2) - kFieldLimit),
            targetHeight,
            (float)(std::rand() % (int)(kFieldLimit * 2) - kFieldLimit)
        };
    }

    // 移動物理
    float currentAccel = state.springStrength * kSpeedMultiplier;
    state.velocity = Math::Add(state.velocity, Math::Multiply(currentAccel, diffToTarget));
    state.velocity = Math::Multiply(state.friction, state.velocity);
    headT.translate = Math::Add(headT.translate, state.velocity);

    // 回転
    if (Math::Length(state.velocity) > 0.01f) {
        Vector3 moveDir = Math::Normalize(state.velocity);
        float targetRotY = std::atan2(moveDir.x, moveDir.z);
        headT.rotate.y += Math::NormalizeAngle(targetRotY - headT.rotate.y) * 0.1f;

        float speed = Math::Length(state.velocity);
        headT.rotate.x = speed * 0.3f;
        headT.rotate.z = -std::sin(globalTimer_ * 2.0f + (float)i) * 0.08f;
    }

    // 攻撃遷移
    if (state.behaviorTimer > kBiteCooldown && distToPlayer < kBiteDistThreshold) {
        state.mode = HeadState::Mode::Biting;
        state.behaviorTimer = 0.0f;
        if (biteStates_[i]) {
            biteStates_[i]->Enter(enemy);
        }
    } else if (state.behaviorTimer > kBeamCooldown && distToPlayer > kBeamDistThreshold) {
        state.mode = HeadState::Mode::Beaming;
        state.behaviorTimer = 0.0f;
    }
}

void Phase2::UpdateBiting(int i, HeadState& state, Enemy* enemy, Player* player, float deltaTime) {
    // 独立したステートクラスに更新を委譲
    if (biteStates_[i]) {
        biteStates_[i]->Update(enemy, player, deltaTime);
        if (biteStates_[i]->IsFinished()) {
            state.mode = HeadState::Mode::Recovering;
            state.behaviorTimer = 0.0f;
            biteStates_[i]->Exit(enemy);
        }
    }
}

void Phase2::UpdateBeaming(int i, HeadState& state, Enemy* enemy, Player* player, float deltaTime) {
    Transform& headT = (i == 0) ? enemy->GetHeadLeftLocalTransform() :
                       (i == 1) ? enemy->GetHeadMidLocalTransform() :
                                  enemy->GetHeadRightLocalTransform();
    Vector3 playerPos = (player) ? player->GetTranslate() : Vector3{ 0, 0, 0 };
    
    enemy->FireBeam();
    EnemyBeam* beam = enemy->GetBeam();
    
    if (beam) {
        if (state.behaviorTimer < 1.0f) {
            beam->SetTelegraphActive(true);
            beam->SetTelegraphThickness(0.1f + state.behaviorTimer * 0.1f);
            beam->Update(headT.translate, playerPos);
        } else if (state.behaviorTimer < 2.5f) {
            beam->SetTelegraphActive(false);
            beam->SetAttackActive(true);
            beam->SetAttackThickness(0.8f);
            beam->Update(headT.translate, playerPos);
        } else {
            beam->SetAttackActive(false);
            state.mode = HeadState::Mode::Recovering;
            state.behaviorTimer = 0.0f;
        }
    }
}

void Phase2::ApplyRepulsion(Enemy* enemy) {
    Transform* transforms[3] = {
        &enemy->GetHeadLeftLocalTransform(),
        &enemy->GetHeadMidLocalTransform(),
        &enemy->GetHeadRightLocalTransform()
    };

    for (int i = 0; i < 3; ++i) {
        for (int j = i + 1; j < 3; ++j) {
            Vector3 diff = Math::Subtract(transforms[i]->translate, transforms[j]->translate);
            float dist = Math::Length(diff);
            if (dist < kRepulsionRadius && dist > 0.001f) {
                float strength = (kRepulsionRadius - dist) / kRepulsionRadius;
                Vector3 push = Math::Multiply(kRepulsionForceScale * strength * strength, Math::Normalize(diff));
                
                headStates_[i].velocity = Math::Add(headStates_[i].velocity, push);
                headStates_[j].velocity = Math::Subtract(headStates_[j].velocity, push);
            }
        }
    }
}
