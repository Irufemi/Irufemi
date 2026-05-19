#include "Phase1_Idle.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include "Core/Math/Math.h"
#include <cmath>

void Phase1_Idle::Update(Enemy* enemy, Player* player, float deltaTime) {
    timer_ += deltaTime;
    float ls = lerpSpeed_;
    Transform& globalT = enemy->GetGlobalTransform();

    // --- 1. プレイヤーの方を向いてじりじり寄る ---
    if (player) {
        Vector3 playerPos = player->GetTranslate();
        Vector3 enemyPos = globalT.translate;
        Vector3 diff = Math::Subtract(playerPos, enemyPos);
        diff.y = 0.0f; 

        float dist = Math::Length(diff);
        if (dist > 0.001f) {
            Vector3 dir = Math::Normalize(diff);
            
            // プレイヤーの方向を少しずつ向く
            float targetYaw = std::atan2(dir.x, dir.z);
            float currentYaw = globalT.rotate.y;
            
            float diffYaw = targetYaw - currentYaw;
            while (diffYaw > 3.14159f) diffYaw -= 2.0f * 3.14159f;
            while (diffYaw < -3.14159f) diffYaw += 2.0f * 3.14159f;

            globalT.rotate.y += diffYaw * trackRotSpeed_ * deltaTime;

            // 前進（向いている方向へ）
            if (!enemy->GetIsSandbagMode()) {
                Vector3 forward = { std::sin(globalT.rotate.y), 0.0f, std::cos(globalT.rotate.y) };
                globalT.translate = Math::Add(globalT.translate, Math::Multiply(creepSpeed_ * deltaTime, forward));
            }
        }
    }

    // --- 2. 頭の歩きモーション（前後スイング＋呼吸） ---
    auto ApplyHeadMotion = [&](Vector3& offset, float phase) {
        // 呼吸の上下
        float waveY = std::sin(timer_ * breathSpeed_ + phase) * breathHeight_;
        // 歩きの前後
        float swingZ = std::sin(timer_ * headSwingSpeed_ + phase) * headSwingDepth_;

        offset.x += (0.0f - offset.x) * returnSpeed_;
        offset.y += (waveY - offset.y) * ls;
        offset.z += (swingZ - offset.z) * ls;
    };

    // 位相をずらして適用 (Mid, Left, Right)
    ApplyHeadMotion(enemy->GetHeadMidOffset(), 0.0f);
    ApplyHeadMotion(enemy->GetHeadLeftOffset(), phaseOffset_);
    ApplyHeadMotion(enemy->GetHeadRightOffset(), phaseOffset_ * 2.0f);

    // --- 3. 胴体のくねくね ---
    for (int i = 0; i < 3; ++i) {
        float fI = (float)(i + 1) / 3.0f;
        // 横のくねくね (Snake-like wiggle)
        float wiggleX = std::sin(timer_ * bodyWiggleSpeed_ - i * 1.5f) * bodyWiggleWidth_ * fI;
        // 上下の波
        float waveY = std::sin(timer_ * breathSpeed_ - (float)(i + 1) * phaseOffset_) * breathHeight_ * 0.5f;

        Vector3& bodyOffset = enemy->GetBodyOffset(i);
        bodyOffset.x += (wiggleX - bodyOffset.x) * ls;
        bodyOffset.y += (waveY - bodyOffset.y) * ls;
        bodyOffset.z += (0.0f - bodyOffset.z) * returnSpeed_;
    }

    globalT.rotate.x += (0.0f - globalT.rotate.x) * returnSpeed_;
}