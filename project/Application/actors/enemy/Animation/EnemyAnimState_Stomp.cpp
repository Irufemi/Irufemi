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
    float holdTime = 1.6f;    // 完全に動きを止める「溜め」
    float jumpTime = 0.25f;   // 一瞬で空へ消える（爆発的速度）

    float endSquat = squatTime;
    float endHold = endSquat + holdTime;
    float endJump = endHold + jumpTime;
    float endHover = endJump + hoverTime_;

    // --- 0. プレイヤーを常に向く（地上にいる間） ---
    if (attackTimer_ < endHold) {
        Vector3 playerPos = player->GetTranslate();
        Vector3 myPos = enemy->GetGlobalTransform().translate;

        // ターゲットへの方向ベクトル
        Vector3 toPlayer = Math::Subtract(playerPos, myPos);
        // Y軸回転（向き）を計算
        enemy->GetGlobalTransform().rotate.y = std::atan2(toPlayer.x, toPlayer.z);
    }

    // --- 1. 地上での予兆（緩急のある屈伸とマルチ振動） ---
    if (attackTimer_ < endSquat) {
        // 【緩急】tの3乗を使うことで、後半にかけて一気に深く沈み込む演出
        float t = attackTimer_ / squatTime;
        float easeIn = t * t * t;
        enemy->GetGlobalTransform().scale.y = initialScaleY_ * (1.0f - easeIn * 0.6f);

        // 【部位別振動】溜めが進むほど激しく、かつ軸をバラバラにする
        float shakeStrength = easeIn * 0.8f;

        // 中央：上下に激しく揺れる（エネルギーの充填）
        enemy->GetHeadMidOffset().y = std::sin(attackTimer_ * 80.0f) * shakeStrength;
        // 左：左右に揺れる
        enemy->GetHeadLeftOffset().x = std::sin(attackTimer_ * 95.0f) * shakeStrength;
        // 右：前後に揺れる
        enemy->GetHeadRightOffset().z = std::cos(attackTimer_ * 110.0f) * shakeStrength;
    }
    // --- 2. 溜めの極致（静寂） ---
    else if (attackTimer_ < endHold) {
        // 飛び上がる直前、全ての振動をピタッと止める（嵐の前の静けさ）
        enemy->GetGlobalTransform().scale.y = initialScaleY_ * 0.4f;
        enemy->GetHeadMidOffset() = { 0,0,0 };
        enemy->GetHeadLeftOffset() = { 0,0,0 };
        enemy->GetHeadRightOffset() = { 0,0,0 };
    }
    // --- 3. 超高速ジャンプ ---
    else if (attackTimer_ < endJump) {
        // 溜めたバネを解放するように、一気に縦長になりながら上昇
        float t = (attackTimer_ - endHold) / jumpTime;
        enemy->GetGlobalTransform().scale.y = initialScaleY_ * (0.4f + t * 1.8f);

        // フレームごとの移動量を大きくして「消えた」ように見せる
        enemy->GetGlobalTransform().translate.y += 3.5f;
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