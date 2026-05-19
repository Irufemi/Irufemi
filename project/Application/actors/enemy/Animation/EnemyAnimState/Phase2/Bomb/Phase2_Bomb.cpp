#include "Phase2_Bomb.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include "Engine/Core/Math/Math.h"
#include <cmath>

void Phase2_Bomb::Enter(Enemy* enemy) {
    timer_ = 0.0f;
    globalTimer_ = 0.0f;
    isFinished_ = false;
    hasThrown_ = false;
}

void Phase2_Bomb::Update(Enemy* enemy, Player* player, float deltaTime) {
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

    // プレイヤーの方向を向く！
    Vector3 toPlayer = Math::Subtract(playerPos, headT->translate);
    headT->rotate.y = std::atan2(toPlayer.x, toPlayer.z);

    if (timer_ < kChargeTime) {
        // 溜めモーション（シェイク）
        float shakeX = std::sin(globalTimer_ * shakeSpeedCharge_ + (float)headIndex_) * kShakeStrength;
        float shakeY = std::cos(globalTimer_ * shakeSpeedCharge_ * 1.1f + (float)headIndex_) * kShakeStrength;
        
        // 少し上に振りかぶる
        float chargeProgress = timer_ / kChargeTime;
        float rise = throwHeightOffset_ * chargeProgress;

        Vector3 shakeOffset = { shakeX, shakeY + rise, 0 };
        headT->translate = Math::Add(basePos_, shakeOffset);

    } else if (timer_ < kThrowTime) {
        // 振り下ろして投げる
        float throwProgress = (timer_ - kChargeTime) / (kThrowTime - kChargeTime);
        float drop = throwHeightOffset_ - (throwHeightOffset_ * 2.0f * throwProgress); // 上がった分から一気に下がる
        
        headT->translate = basePos_;
        headT->translate.y += drop;

        if (!hasThrown_) {
            // 爆弾を発射
            enemy->FireBomb(headIndex_, playerPos);
            hasThrown_ = true;
        }
    } else {
        // 終了
        headT->translate = basePos_;
        isFinished_ = true;
    }
}

void Phase2_Bomb::Exit(Enemy* enemy) {
    // 状態抜けたら特にすることはない（爆弾は独自に飛んでいくため）
}
