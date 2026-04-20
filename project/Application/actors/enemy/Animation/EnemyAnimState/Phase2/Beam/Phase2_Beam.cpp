#include "Phase2_Beam.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include "Engine/Core/Math/Math.h"
#include "Beam/EnemyBeam.h"
#include <cmath>
#include <algorithm>

void Phase2_Beam::Enter(Enemy* enemy) {
    timer_ = 0.0f;
    globalTimer_ = 0.0f;
    isFinished_ = false;
    attackTarget_ = { 0, 0, 0 };
}

void Phase2_Beam::Update(Enemy* enemy, Player* player, float deltaTime) {
    if (!enemy) return;

    if (timer_ == 0.0f) {
        Transform* headT = nullptr;
        if (headIndex_ == 0) headT = &enemy->GetHeadLeftLocalTransform();
        else if (headIndex_ == 1) headT = &enemy->GetHeadMidLocalTransform();
        else headT = &enemy->GetHeadRightLocalTransform();
        if (headT) {
            basePos_ = headT->translate;
        }
    }

    timer_ += deltaTime;
    globalTimer_ += deltaTime; 
    Vector3 playerPos = (player) ? player->GetTranslate() : Vector3{ 0, 0, 0 };

    Transform* headT = nullptr;
    if (headIndex_ == 0) headT = &enemy->GetHeadLeftLocalTransform();
    else if (headIndex_ == 1) headT = &enemy->GetHeadMidLocalTransform();
    else headT = &enemy->GetHeadRightLocalTransform();

    if (!headT) return;

    enemy->FireBeam();
    EnemyBeam* beam = enemy->GetBeam(headIndex_);
    if (!beam) return;

    // Phase2時点では、headT->translate がすでにワールド座標として振る舞っています
    // （Enemy::Update内での描画処理でそのように分岐されているため）
    // そのため、頭の中心から上にオフセット(headExtensionY_)を足して、口元/頭頂からビームが出るように調整します
    Vector3 headWorldPos = { headT->translate.x, headT->translate.y + headExtensionY_, headT->translate.z };

    if (timer_ < kBeamChargeTime) {
        // シェイク動作と追尾
        float shakeX = std::sin(globalTimer_ * shakeSpeedCharge_ + (float)headIndex_) * kBeamShakeStrength;
        float shakeY = std::cos(globalTimer_ * shakeSpeedCharge_ * 1.1f + (float)headIndex_) * kBeamShakeStrength;
        
        // プレイヤーの方向を向く
        Vector3 toPlayer = Math::Subtract(playerPos, headT->translate);
        headT->rotate.y = std::atan2(toPlayer.x, toPlayer.z);
        float distXZ = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z);
        headT->rotate.x = -std::atan2(toPlayer.y, distXZ);

        // 中心位置からシェイクさせる
        Vector3 shakeOffset = { shakeX, shakeY, 0 };
        headT->translate = Math::Add(basePos_, shakeOffset);

        beam->SetTelegraphActive(true);
        beam->SetTelegraphThickness(telegraphThicknessBase_ + timer_ * telegraphThicknessGrow_);
        beam->Update(headWorldPos, playerPos);
    } else if (timer_ < kBeamWaitTime) {
        // 停止・ロックオン（シェイクを止めて定位置に戻す）
        headT->translate = basePos_;
        if (timer_ - deltaTime < kBeamChargeTime) {
            attackTarget_ = playerPos; 
        }
        beam->SetTelegraphActive(true);
        beam->SetTelegraphThickness(telegraphThicknessWait_);
        beam->Update(headWorldPos, attackTarget_);
    } else if (timer_ < kBeamFireTime) {
        // 本射
        beam->SetTelegraphActive(false);
        beam->SetAttackActive(true);
        beam->SetAttackThickness(attackThickness_);
        beam->Update(headWorldPos, attackTarget_);
        
        float fireShake = std::sin(globalTimer_ * shakeSpeedFire_) * fireShakeStrength_;
        headT->translate.y = basePos_.y + fireShake;
    } else {
        // 終了
        headT->translate = basePos_;
        beam->SetAttackActive(false);
        isFinished_ = true;
    }
}

void Phase2_Beam::Exit(Enemy* enemy) {
    if (auto* beam = enemy->GetBeam(headIndex_)) {
        beam->SetTelegraphActive(false);
        beam->SetAttackActive(false);
    }
}
