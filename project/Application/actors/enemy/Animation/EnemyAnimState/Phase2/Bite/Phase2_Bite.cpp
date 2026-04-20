#include "Phase2_Bite.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include "Engine/Core/Math/Math.h"
#include <cmath>
#include <algorithm>

void Phase2_Bite::Enter(Enemy* enemy) {
    timer_ = 0.0f;
    isFinished_ = false;
    isTargetLocked_ = false;
    attackTarget_ = { 0, 0, 0 };
}

void Phase2_Bite::Update(Enemy* enemy, Player* player, float deltaTime) {
    if (!enemy) return;

    timer_ += deltaTime;
    Vector3 playerPos = (player) ? player->GetTranslate() : Vector3{ 0, 0, 0 };

    // 操作対象の首のトランスフォームを取得
    Transform* headT = nullptr;
    if (headIndex_ == 0) headT = &enemy->GetHeadLeftLocalTransform();
    else if (headIndex_ == 1) headT = &enemy->GetHeadMidLocalTransform();
    else headT = &enemy->GetHeadRightLocalTransform();

    if (!headT) return;

    if (timer_ < kOrbitTime) {
        // --- 旋回中は常にプレイヤーを向く ---
        Vector3 toPlayer = Math::Subtract(playerPos, headT->translate);
        headT->rotate.y = std::atan2(toPlayer.x, toPlayer.z);
        float distXZ = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z);
        headT->rotate.x = -std::atan2(toPlayer.y, distXZ);

        // 旋回しながら近づく
        float angleOffset = (float)headIndex_ * (Math::PI * 2.0f / 3.0f);
        float currentAngle = timer_ * kOrbitSpeed + angleOffset;
        
        Vector3 targetPos = playerPos;
        targetPos.x += std::cos(currentAngle) * kOrbitRadius;
        targetPos.z += std::sin(currentAngle) * kOrbitRadius;
        targetPos.y += 2.0f;

        headT->translate.x += (targetPos.x - headT->translate.x) * 0.1f;
        headT->translate.y += (targetPos.y - headT->translate.y) * 0.1f;
        headT->translate.z += (targetPos.z - headT->translate.z) * 0.1f;
    }
    else if (timer_ < kOrbitTime + kStopTime) {
        if (!isTargetLocked_) {
            // この瞬間にターゲット位置を確定させる
            attackTarget_ = playerPos;
            
            // ターゲットの方にしっかり向きを固定する
            Vector3 toTarget = Math::Subtract(attackTarget_, headT->translate);
            headT->rotate.y = std::atan2(toTarget.x, toTarget.z);
            float distXZ = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
            headT->rotate.x = -std::atan2(toTarget.y, distXZ);

            isTargetLocked_ = true;
        }

        // --- 位置の移動を完全に停止する（明確な隙の提示） ---
        // わずかに顔を上げる（口を開ける溜め）動作だけ行い、translateはいじらない。
        // こうすることで「場所を決めた」「いまからここに噛みつく」という予備動作になる。
        Vector3 toTarget = Math::Subtract(attackTarget_, headT->translate);
        float distXZ = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
        float targetRotX = -std::atan2(toTarget.y, distXZ) - 0.5f; // 上を向く
        
        headT->rotate.x += (targetRotX - headT->rotate.x) * 0.15f;
    }
    else if (timer_ < kOrbitTime + kStopTime + kRushTime) {
        // 突進：溜めた顔を勢いよく下に振り下ろしながら目標へ飛び込む
        Vector3 toTarget = Math::Subtract(attackTarget_, headT->translate);
        headT->rotate.x += (-std::atan2(toTarget.y, std::sqrt(toTarget.x*toTarget.x + toTarget.z*toTarget.z)) + 0.3f - headT->rotate.x) * 0.2f;

        Vector3 rushDir = Math::Normalize(Math::Subtract(attackTarget_, headT->translate));
        headT->translate = Math::Add(headT->translate, Math::Multiply(kRushSpeed, rushDir));
    }
    else {
        isFinished_ = true;
    }
}

void Phase2_Bite::Exit(Enemy* enemy) {
    // 特になし
}
