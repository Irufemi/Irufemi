#include "EnemyAI.h"
#include "Enemy.h"
#include <cmath>

EnemyAI::~EnemyAI() {}

void EnemyAI::Initialize(Enemy* enemy) {
    enemy_ = enemy;
    timer_ = kZeroThreshold;
    attackWaitTimer_ = kZeroThreshold;
    isWaitingForNextAttack_ = false;
    isFirstAttackStarted_ = false; // 初期化
}

void EnemyAI::Update(float deltaTime) {
    if (!enemy_) return;

    // 1. 最初の待機時間を消化
    if (!isFirstAttackStarted_) {
        timer_ += deltaTime;
        if (timer_ >= startDelay_) {
            isFirstAttackStarted_ = true;
            enemy_->SetState(EnemyState::Attack_Beam);
        }
        return;
    }

    // 2. 次の攻撃までの待機処理 (待機フラグが立っている間)
    if (isWaitingForNextAttack_) {
        attackWaitTimer_ += deltaTime;
        if (attackWaitTimer_ >= attackInterval_) {
            isWaitingForNextAttack_ = false;

            // ★交互に切り替えるロジック
            if (nextIsStomp_) {
                enemy_->SetState(EnemyState::Attack_Stomp);
            } else {
                enemy_->SetState(EnemyState::Attack_Beam);
            }
            nextIsStomp_ = !nextIsStomp_; // 次回のために反転
        }

    // 3. 攻撃中：アニメーションが完了したか監視
    } else {
        if (enemy_->GetAnimation()->HasFinishedAttack()) {
            enemy_->SetState(EnemyState::Idle);
            isWaitingForNextAttack_ = true;
            attackWaitTimer_ = kZeroThreshold;

            // 次の攻撃を交互にするためにフラグを反転
            nextIsStomp_ = !nextIsStomp_;
        }
    }
}