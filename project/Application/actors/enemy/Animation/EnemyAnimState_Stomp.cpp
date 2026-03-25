#include "EnemyAnimState_Stomp.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include "core/math/geometry/Math.h"

void EnemyAnimState_Stomp::Enter(Enemy* enemy) {
    attackTimer_ = 0.0f;
    hasFinishedAttack_ = false;
    hasTeleported_ = false;
    hasHitGround_ = false;
}

void EnemyAnimState_Stomp::Update(Enemy* enemy, Player* player, float deltaTime) {
    attackTimer_ += deltaTime;

    float endAnticipation = anticipationTime_;
    float endHover = endAnticipation + hoverTime_;

    // 1. 地上での予兆（ビームの時のような振動）
    if (attackTimer_ < endAnticipation) {
        float shake = std::sin(attackTimer_ * 50.0f) * 0.2f;
        enemy->GetHeadMidOffset().x += shake;
    }
    // 2. プレイヤーの頭上へテレポート
    else if (attackTimer_ < endHover) {
        if (!hasTeleported_) {
            Vector3 pPos = player->GetTranslate();
            // プレイヤーの真上 stompHeight_ の位置へ移動
            enemy->GetGlobalTransform().translate = { pPos.x, pPos.y + stompHeight_, pPos.z };
            hasTeleported_ = true;
        }
        // 空中でプルプル震えて「落ちるぞ！」という予兆
        float shake = std::sin(attackTimer_ * 60.0f) * 0.3f;
        enemy->GetGlobalTransform().translate.x += shake;
    }
    // 3. 落下
    else if (!hasHitGround_) {
        Vector3& pos = enemy->GetGlobalTransform().translate;
        pos.y -= dropSpeed_; // 急降下

        // 地面（とりあえずY=3.0と仮定）に着いたら衝撃波
        if (pos.y <= 3.0f) {
            pos.y = 3.0f;
            hasHitGround_ = true;

            enemy->FireStomp(pos);

            attackTimer_ = endHover; // タイマーを硬直フェーズの開始に合わせる

            // TODO: ここで衝撃波(Shockwave)を発生させる！
            // enemy->FireShockwave(); 
        }
    }
    // 4. 着地硬直
    else {
        float t = attackTimer_ - endHover;
        if (t >= recoveryTime_) {
            hasFinishedAttack_ = true;
        }
    }
}

void EnemyAnimState_Stomp::Exit(Enemy* enemy) {
    // 振動などのオフセットをリセット
    enemy->GetHeadMidOffset() = { 0,0,0 };
}