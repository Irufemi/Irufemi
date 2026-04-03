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
    float totalRecoveryTime = landSquatHoldTime_ + landRiseTime_;

    // --- 0. プレイヤーを「補間しながら」向く（地上予兆中） ---
    if (attackTimer_ < endHold) {
        Vector3 playerPos = player->GetTranslate();
        Vector3 myPos = enemy->GetGlobalTransform().translate;
        Vector3 toPlayer = Math::Subtract(playerPos, myPos);

        // 目標となる角度（ラジアン）
        float targetAngle = std::atan2(toPlayer.x, toPlayer.z);
        // 現在の角度
        float currentAngle = enemy->GetGlobalTransform().rotate.y;

        // 角度の差分を計算（180度を超えた時の最短ルート計算）
        float angleDiff = targetAngle - currentAngle;
        while (angleDiff > Math::PI) angleDiff -= Math::PI * 2;
        while (angleDiff < -Math::PI) angleDiff += Math::PI * 2;

        // 補間して回転を更新
        // deltaTimeとスピードを掛けることで、フレームレートに依存せず滑らかに回転
        enemy->GetGlobalTransform().rotate.y += angleDiff * rotationInterpolationSpeed_ * deltaTime;
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

        // 落下中は少し縦に伸ばすとスピード感が出ます
        enemy->GetGlobalTransform().scale.y = initialScaleY_ * 1.2f;

        if (pos.y <= groundY_) {
            pos.y = groundY_;
            hasHitGround_ = true;

            // 【着地瞬間】一気に押し潰す！
            enemy->GetGlobalTransform().scale.y = initialScaleY_ * landSquatScale_;

            enemy->FireStomp(pos); // 衝撃波エフェクト
            attackTimer_ = endHover; // タイマーを着地硬直の開始時間に合わせる
        }
    }
    // --- 6. 着地硬直（ずっしり復帰） ---
    else {
        float t = attackTimer_ - endHover;

        // タイムラインの目印を計算
        float endSquatDown = landSquatDownTime_;
        float endSquatHold = endSquatDown + landSquatHoldTime_;
        float endTotal = endSquatHold + landRiseTime_;

        if (t < endSquatDown) {
            // 【ステップ1：衝撃の伝播】ググッと潰れていく
            float subT = t / landSquatDownTime_;
            // 線形ではなく、勢いよく潰れ始めてゆっくり止まるイージング
            float easeOut = 1.0f - std::pow(1.0f - subT, 2.0f);

            // 落下中の1.2倍からlandSquatScale_へ
            float startScale = 1.2f;
            enemy->GetGlobalTransform().scale.y = initialScaleY_ * (startScale + easeOut * (landSquatScale_ - startScale));

        } else if (t < endSquatHold) {
            // 【ステップ2：溜め】最大まで潰れた状態で微振動
            enemy->GetGlobalTransform().scale.y = initialScaleY_ * landSquatScale_;

            // 地面にめり込む微振動
            float microShake = 0.15f;
            enemy->GetGlobalTransform().translate.x += std::sin(t * 160.0f) * microShake;

        } else if (t < endTotal) {
            // 【ステップ3：ずっしり復帰】
            float riseT = (t - endSquatHold) / landRiseTime_;

            // 非常に重そうに立ち上がるためのイージング (4次関数)
            float easeOutRise = 1.0f - std::pow(1.0f - riseT, 4.0f);

            enemy->GetGlobalTransform().scale.y = initialScaleY_ * (landSquatScale_ + easeOutRise * (1.0f - landSquatScale_));

            // 復帰中の余韻振動（徐々に弱まる）
            float shake = std::sin(riseT * 20.0f) * (1.0f - riseT) * 0.1f;
            enemy->GetHeadMidOffset().y = shake;

        } else {
            // 全工程終了
            enemy->GetGlobalTransform().scale.y = initialScaleY_;
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