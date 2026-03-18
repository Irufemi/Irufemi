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
        return;
    }

    // 2. 次の攻撃までの待機処理 (待機フラグが立っている間)
    if (isWaitingForNextAttack_) {
        attackWaitTimer_ += 1.0f / 60.0f;
        if (attackWaitTimer_ >= attackInterval_) {
            isWaitingForNextAttack_ = false;
            enemy_->SetState(EnemyState::Attack_Beam); // ここでビーム状態へ
        }
    }
    // 3. 攻撃中：アニメーションが完了したか監視
    else {
        if (enemy_->GetAnimation()->HasFinishedAttack()) {
            // 攻撃が終わったので待機モードへ移行
            isWaitingForNextAttack_ = true;
            attackWaitTimer_ = 0.0f;
            enemy_->SetState(EnemyState::Idle);
        }
    }
}