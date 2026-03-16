#include "EnemyAI.h"
#include "Enemy.h"
#include <cmath>

EnemyAI::~EnemyAI() {}

void EnemyAI::Initialize(Enemy* enemy) {
    enemy_ = enemy;
    timer_ = 0.0f;
    attackWaitTimer_ = 0.0f;
    isWaitingForNextAttack_ = false;
    isFirstAttackStarted_ = false; // 初期化
}

void EnemyAI::Update() {
    if (!enemy_) return;

    // 1. 最初の待機時間を消化
    if (!isFirstAttackStarted_) {
        timer_ += 1.0f / 60.0f;
        if (timer_ >= startDelay_) {
            isFirstAttackStarted_ = true;
            enemy_->SetState(EnemyState::Attack_Beam);
        }
        return; // 最初の待機中は以下の処理（通常のループ）を行わない
    }

    // 2. 通常の攻撃サイクル（アニメーション完了待ち）
    if (enemy_->GetAnimation()->HasFinishedAttack()) {
        enemy_->GetAnimation()->ResetAttackFinished();
        isWaitingForNextAttack_ = true;
        attackWaitTimer_ = 0.0f;
    }

    // 3. 次の攻撃までの待機処理
    if (isWaitingForNextAttack_) {
        attackWaitTimer_ += 1.0f / 60.0f;
        if (attackWaitTimer_ >= attackInterval_) {
            enemy_->SetState(EnemyState::Attack_Beam);
            isWaitingForNextAttack_ = false;
        }
    }
}