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

    // 開始時のスケールを保存（屈伸のリセット用）
    initialScaleY_ = enemy->GetGlobalTransform().scale.y;
}

void EnemyAnimState_Stomp::Update(Enemy* enemy, Player* player, float deltaTime) {
    attackTimer_ += deltaTime;

    // --- フェーズ時間設定 ---
    float squatTime = 0.7f;   // じっくり深く沈み込む
    float holdTime = 1.2f;    // 完全に動きを止める「溜め」
    float jumpTime = 0.35f;   // 一瞬で空へ消える（爆発的速度）

    float endSquat = squatTime;
    float endHold = endSquat + holdTime;
    float endJump = endHold + jumpTime;
    float endHover = endJump + hoverTime_;

    // --- 0. プレイヤーを常に向く（地上にいる間） ---
    if (attackTimer_ < endHold) {
        Vector3 playerPos = player->GetTranslate();
        Vector3 myPos = enemy->GetGlobalTransform().translate;
        Vector3 toPlayer = Math::Subtract(playerPos, myPos);
        enemy->GetGlobalTransform().rotate.y = std::atan2(toPlayer.x, toPlayer.z);
    }

    // --- 1. 地上での予兆（緩急のある屈伸とマルチ振動） ---
    if (attackTimer_ < endSquat) {
        float t = attackTimer_ / squatTime;
        float easeIn = t * t * t;
        enemy->GetGlobalTransform().scale.y = initialScaleY_ * (1.0f - easeIn * 0.5f);

        // 徐々に強くなる振動
        float shakeStrength = easeIn * 0.5f;
        enemy->GetHeadMidOffset().y = std::sin(attackTimer_ * 60.0f) * shakeStrength;
        enemy->GetHeadLeftOffset().x = std::sin(attackTimer_ * 75.0f) * shakeStrength;
        enemy->GetHeadRightOffset().z = std::cos(attackTimer_ * 80.0f) * shakeStrength;
    }
    // --- 2. 溜めの極致（高周波の微振動） ---
    else if (attackTimer_ < endHold) {
        // 圧縮された状態（一番縮んだ状態）を維持
        enemy->GetGlobalTransform().scale.y = initialScaleY_ * 0.5f;

        // 【ここを修正】完全に止めず、超高速で細かく震わせる
        // 周波数を150.0f以上に上げることで「キチキチ」とした限界感を出す
        float microShake = 0.15f;
        float speed = 180.0f;

        enemy->GetHeadMidOffset().y = std::sin(attackTimer_ * speed) * microShake;
        enemy->GetHeadLeftOffset().x = std::cos(attackTimer_ * (speed + 10.0f)) * microShake;
        enemy->GetHeadRightOffset().z = std::sin(attackTimer_ * (speed - 5.0f)) * microShake;

        // さらに、体全体（Transformの座標）もわずかに震わせるとより「力」を感じます
        enemy->GetGlobalTransform().translate.x += std::sin(attackTimer_ * 200.0f) * 0.05f;
    }
    // --- 3. 爆発的ジャンプ ---
    else if (attackTimer_ < endJump) {
        // 溜めた微振動を一気に解放して上昇
        float t = (attackTimer_ - endHold) / jumpTime;
        enemy->GetGlobalTransform().scale.y = initialScaleY_ * (0.5f + t * 2.0f);
        enemy->GetGlobalTransform().translate.y += 4.0f; // より速く！

        // ジャンプの瞬間はオフセットをリセット
        enemy->GetHeadMidOffset() = { 0,0,0 };
        enemy->GetHeadLeftOffset() = { 0,0,0 };
        enemy->GetHeadRightOffset() = { 0,0,0 };
    }
    // --- 4. プレイヤー頭上への回り込み ---
    else if (attackTimer_ < endHover) {
        if (!hasTeleported_) {
            Vector3 pPos = player->GetTranslate();
            enemy->GetGlobalTransform().translate = { pPos.x, pPos.y + stompHeight_, pPos.z };
            enemy->GetGlobalTransform().scale.y = initialScaleY_; // スケール正常化
            hasTeleported_ = true;
        }
        // 空中で落下直前の不気味な微振動
        float shake = std::sin(attackTimer_ * 120.0f) * 0.3f;
        enemy->GetGlobalTransform().translate.x += shake;
    }
    // --- 5. 落下・衝撃波（既存の攻撃処理） ---
    else if (!hasHitGround_) {
        Vector3& pos = enemy->GetGlobalTransform().translate;
        pos.y -= dropSpeed_;

        if (pos.y <= 3.0f) {
            pos.y = 3.0f;
            hasHitGround_ = true;
            enemy->FireStomp(pos); // EnemyStompEffectsの発火
            attackTimer_ = endHover;
        }
    }
    // --- 6. 着地後の硬直 ---
    else {
        float t = attackTimer_ - endHover;
        if (t >= recoveryTime_) {
            hasFinishedAttack_ = true;
        }
    }
}

void EnemyAnimState_Stomp::Exit(Enemy* enemy) {
    // 全ての変形・振動をリセットして次の状態へ渡す
    enemy->GetHeadMidOffset() = { 0,0,0 };
    enemy->GetHeadLeftOffset() = { 0,0,0 };
    enemy->GetHeadRightOffset() = { 0,0,0 };
    enemy->GetGlobalTransform().scale.y = initialScaleY_;
}