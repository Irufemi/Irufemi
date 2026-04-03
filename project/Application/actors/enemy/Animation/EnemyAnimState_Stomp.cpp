#include "EnemyAnimState_Stomp.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include "core/math/geometry/Math.h"
#include <cmath>

void EnemyAnimState_Stomp::Enter(Enemy* enemy) {
    attackTimer_ = 0.0f;
    hasFinishedAttack_ = false;
    hasTeleported_ = false;
    hasHitGround_ = false;
    initialScaleY_ = enemy->GetGlobalTransform().scale.y;
}

void EnemyAnimState_Stomp::Update(Enemy* enemy, Player* player, float deltaTime) {
    attackTimer_ += deltaTime;

    // フェーズ境界の計算
    float endSquat = squatTime_;
    float endHold = endSquat + holdTime_;
    float endJump = endHold + jumpTime_;
    float endHover = endJump + hoverTime_;

    // --- 0. プレイヤーを常に向く（地上予兆中） ---
    if (attackTimer_ < endHold) {
        Vector3 toPlayer = Math::Subtract(player->GetTranslate(), enemy->GetGlobalTransform().translate);
        enemy->GetGlobalTransform().rotate.y = std::atan2(toPlayer.x, toPlayer.z);
    }

    // --- 1. 地上での予兆（屈伸） ---
    if (attackTimer_ < endSquat) {
        float t = attackTimer_ / squatTime_;
        float easeIn = t * t * t;

        // スケール変更
        enemy->GetGlobalTransform().scale.y = initialScaleY_ * (1.0f - (easeIn * (1.0f - maxSquatScale_)));

        // 部位別振動
        float currentShake = easeIn * squatShakeStrength_;
        enemy->GetHeadMidOffset().y = std::sin(attackTimer_ * 60.0f) * currentShake;
        enemy->GetHeadLeftOffset().x = std::sin(attackTimer_ * 75.0f) * currentShake;
        enemy->GetHeadRightOffset().z = std::cos(attackTimer_ * 80.0f) * currentShake;
    }
    // --- 2. 溜めの極致（高周波微振動） ---
    else if (attackTimer_ < endHold) {
        enemy->GetGlobalTransform().scale.y = initialScaleY_ * maxSquatScale_;

        // ヘッダのパラメータを使用した微振動
        enemy->GetHeadMidOffset().y = std::sin(attackTimer_ * holdShakeSpeed_) * holdShakeStrength_;
        enemy->GetHeadLeftOffset().x = std::cos(attackTimer_ * (holdShakeSpeed_ + 10.0f)) * holdShakeStrength_;
        enemy->GetHeadRightOffset().z = std::sin(attackTimer_ * (holdShakeSpeed_ - 5.0f)) * holdShakeStrength_;

        // 全体の共振
        enemy->GetGlobalTransform().translate.x += std::sin(attackTimer_ * 200.0f) * 0.05f;
    }
    // --- 3. 爆発的ジャンプ ---
    else if (attackTimer_ < endJump) {
        float t = (attackTimer_ - endHold) / jumpTime_;
        // 溜めスケールから縦伸びスケールへ
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
    }
    // --- 5. 落下 ---
    else if (!hasHitGround_) {
        Vector3& pos = enemy->GetGlobalTransform().translate;
        pos.y -= dropSpeed_;

        if (pos.y <= groundY_) {
            pos.y = groundY_;
            hasHitGround_ = true;
            enemy->FireStomp(pos);
            attackTimer_ = endHover;
        }
    }
    // --- 6. 着地硬直 ---
    else {
        float t = attackTimer_ - endHover;
        if (t >= recoveryTime_) {
            hasFinishedAttack_ = true;
        }
    }
}

void EnemyAnimState_Stomp::Exit(Enemy* enemy) {
    enemy->GetHeadMidOffset() = { 0,0,0 };
    enemy->GetHeadLeftOffset() = { 0,0,0 };
    enemy->GetHeadRightOffset() = { 0,0,0 };
    enemy->GetGlobalTransform().scale.y = initialScaleY_;
}