#include "Bite.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include "Engine/Core/Math/Math.h"
#include <cmath>
#include <algorithm>

void Bite::Enter(Enemy* enemy) {
    timer_ = 0.0f;
    isFinished_ = false;
}

void Bite::Update(Enemy* enemy, Player* player, float deltaTime) {
    if (!enemy) return;

    timer_ += deltaTime;
    Vector3 playerPos = (player) ? player->GetTranslate() : Vector3{ 0, 0, 0 };

    // 操作対象の首のトランスフォームを取得
    Transform* headT = nullptr;
    if (headIndex_ == 0) headT = &enemy->GetHeadLeftLocalTransform();
    else if (headIndex_ == 1) headT = &enemy->GetHeadMidLocalTransform();
    else headT = &enemy->GetHeadRightLocalTransform();

    if (!headT) return;

    // --- プレイヤーをロックオン ---
    Vector3 toPlayer = Math::Subtract(playerPos, headT->translate);
    headT->rotate.y = std::atan2(toPlayer.x, toPlayer.z);
    float distXZ = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z);
    headT->rotate.x = -std::atan2(toPlayer.y, distXZ);

    // --- フェーズ別挙動 ---
    if (timer_ < kChargeTime) {
        // 溜め：少し引く
        Vector3 backDir = Math::Normalize(Math::Subtract(headT->translate, playerPos));
        headT->translate = Math::Add(headT->translate, Math::Multiply(kBackStepSpeed, backDir));
    }
    else if (timer_ < kRushTime) {
        // 突進
        Vector3 rushDir = Math::Normalize(Math::Subtract(playerPos, headT->translate));
        headT->translate = Math::Add(headT->translate, Math::Multiply(kRushSpeed, rushDir));
    }
    else {
        isFinished_ = true;
    }
}

void Bite::Exit(Enemy* enemy) {
    // 特になし
}
