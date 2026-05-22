#include "Phase1_Stomp.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include "Core/Math/Math.h"
#include <cmath>

void Phase1_Stomp::Enter(Enemy* enemy) {
    attackTimer_ = 0.0f;
    hasFinishedAttack_ = false;
    hasTeleported_ = false;
    hasHitGround_ = false;
    initialScaleY_ = enemy->GetGlobalTransform().scale.y;
}

void Phase1_Stomp::Update(Enemy* enemy, Player* player, float deltaTime) {
    attackTimer_ += deltaTime;

    // フェーズ境界の計算
    float endSquat = squatTime_;
    float endHold = endSquat + holdTime_;
    float endJump = endHold + jumpTime_;
    float endHover = endJump + hoverTime_;

    // --- 0. プレイヤーを補間しながら向く ---
    if (attackTimer_ < endHold) {
        Vector3 playerPos = player->GetTranslate();
        Vector3 myPos = enemy->GetGlobalTransform().translate;
        Vector3 toPlayer = Math::Subtract(playerPos, myPos);

        float targetAngle = std::atan2(toPlayer.x, toPlayer.z);
        float currentAngle = enemy->GetGlobalTransform().rotate.y;

        float angleDiff = targetAngle - currentAngle;
        while (angleDiff > Math::PI) angleDiff -= Math::PI * 2;
        while (angleDiff < -Math::PI) angleDiff += Math::PI * 2;

        enemy->GetGlobalTransform().rotate.y += angleDiff * rotationInterpolationSpeed_ * deltaTime;
    }

    // --- 1. 予兆（屈伸） ---
    if (attackTimer_ < endSquat) {
        float t = attackTimer_ / squatTime_;
        float easeIn = t * t * t;

        enemy->GetGlobalTransform().scale.y = initialScaleY_ * (1.0f - (easeIn * (1.0f - maxSquatScale_)));

        float currentShake = easeIn * squatShakeStrength_;
        enemy->GetHeadMidOffset().y = std::sin(attackTimer_ * 60.0f) * currentShake;
        enemy->GetHeadLeftOffset().x = std::sin(attackTimer_ * 75.0f) * currentShake;
        enemy->GetHeadRightOffset().z = std::cos(attackTimer_ * 80.0f) * currentShake;
    }
    // --- 2. 溜め ---
    else if (attackTimer_ < endHold) {
        enemy->GetGlobalTransform().scale.y = initialScaleY_ * maxSquatScale_;

        enemy->GetHeadMidOffset().y = std::sin(attackTimer_ * holdShakeSpeed_) * holdShakeStrength_;
        enemy->GetHeadLeftOffset().x = std::cos(attackTimer_ * (holdShakeSpeed_ + 10.0f)) * holdShakeStrength_;
        enemy->GetHeadRightOffset().z = std::sin(attackTimer_ * (holdShakeSpeed_ - 5.0f)) * holdShakeStrength_;

        enemy->GetGlobalTransform().translate.x += std::sin(attackTimer_ * 200.0f) * 0.05f;
    }
    // --- 3. ジャンプ ---
    else if (attackTimer_ < endJump) {
        float t = (attackTimer_ - endHold) / jumpTime_;
        enemy->GetGlobalTransform().scale.y = initialScaleY_ * (maxSquatScale_ + t * (jumpStretchScale_ - maxSquatScale_));
        enemy->GetGlobalTransform().translate.y += jumpUpSpeed_;

        enemy->GetHeadMidOffset() = { 0,0,0 };
        enemy->GetHeadLeftOffset() = { 0,0,0 };
        enemy->GetHeadRightOffset() = { 0,0,0 };
    }
    // --- 4. プレイヤー頭上待機 ---
    else if (attackTimer_ < endHover) {
        if (!hasTeleported_) {
            Vector3 pPos = player->GetTranslate();
            enemy->GetGlobalTransform().translate = { pPos.x, pPos.y + stompHeight_, pPos.z };
            enemy->GetGlobalTransform().scale.y = initialScaleY_;
            hasTeleported_ = true;
        }
        float shake = std::sin(attackTimer_ * 120.0f) * 0.3f;
        enemy->GetGlobalTransform().translate.x += shake;

        // 警告演出をアクティブにする
        enemy->SetWarningActive(true);
    }
    // --- 5. 落下 ---
    else if (!hasHitGround_) {
        Vector3& pos = enemy->GetGlobalTransform().translate;
        pos.y -= dropSpeed_;
        enemy->GetGlobalTransform().scale.y = initialScaleY_ * 1.2f;

        // 落下中も警告演出を継続
        enemy->SetWarningActive(true);

        if (pos.y <= groundY_) {
            pos.y = groundY_;
            hasHitGround_ = true;
            enemy->GetGlobalTransform().scale.y = initialScaleY_ * landSquatScale_;
            enemy->FireStomp(pos); 
            attackTimer_ = endHover; 

            // 着地（激突）したため、警告を終了する
            enemy->SetWarningActive(false);
        }
    }
    // --- 6. 着地硬直 ---
    else {
        float t = attackTimer_ - endHover;
        float endSquatDown = landSquatDownTime_;
        float endSquatHold = endSquatDown + landSquatHoldTime_;
        float endTotal = endSquatHold + landRiseTime_;

        if (t < endSquatDown) {
            float subT = t / landSquatDownTime_;
            float easeOut = 1.0f - std::pow(1.0f - subT, 2.0f);
            float startScale = 1.2f;
            enemy->GetGlobalTransform().scale.y = initialScaleY_ * (startScale + easeOut * (landSquatScale_ - startScale));
        } else if (t < endSquatHold) {
            enemy->GetGlobalTransform().scale.y = initialScaleY_ * landSquatScale_;
            enemy->GetGlobalTransform().translate.x += std::sin(t * 160.0f) * 0.15f;
        } else if (t < endTotal) {
            float riseT = (t - endSquatHold) / landRiseTime_;
            float easeOutRise = 1.0f - std::pow(1.0f - riseT, 4.0f);
            enemy->GetGlobalTransform().scale.y = initialScaleY_ * (landSquatScale_ + easeOutRise * (1.0f - landSquatScale_));
            float shake = std::sin(riseT * 20.0f) * (1.0f - riseT) * 0.1f;
            enemy->GetHeadMidOffset().y = shake;
        } else {
            enemy->GetGlobalTransform().scale.y = initialScaleY_;
            hasFinishedAttack_ = true;
        }
    }
}

void Phase1_Stomp::Exit(Enemy* enemy) {
    enemy->GetHeadMidOffset() = { 0,0,0 };
    enemy->GetHeadLeftOffset() = { 0,0,0 };
    enemy->GetHeadRightOffset() = { 0,0,0 };
    enemy->GetGlobalTransform().scale.y = initialScaleY_;
    enemy->SetWarningActive(false); // 安全対策として警告を確実に終了
}