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
    nextAttackIndex_ = 0;
}

void EnemyAI::Update(float deltaTime) {
    if (!enemy_) return;

    // フェーズ2中は、個別の首が自律行動するため既存のAIルーチンを停止する
    if (enemy_->GetState() == EnemyState::Phase2) {
        return;
    }

    // 1. 最初の待機時間を消化
    if (!isFirstAttackStarted_) {
        timer_ += deltaTime;
        if (timer_ >= startDelay_) {
            isFirstAttackStarted_ = true;
            enemy_->SetState(EnemyState::Attack_Tackle);
        }
        return;
    }

    // 2. 次の攻撃までの待機処理 (待機フラグが立っている間)
    if (isWaitingForNextAttack_) {
        attackWaitTimer_ += deltaTime;
        if (attackWaitTimer_ >= attackInterval_) {
            isWaitingForNextAttack_ = false;
            // ★順番に切り替えるロジック (0: Beam, 1: Stomp, 2: Neck, 3: Tackle)
            if (nextAttackIndex_ == 0) {
                enemy_->SetState(EnemyState::Attack_Beam);
            } else if (nextAttackIndex_ == 1) {
                enemy_->SetState(EnemyState::Attack_Stomp);
            } else if (nextAttackIndex_ == 2) {
                enemy_->SetState(EnemyState::Attack_Neck);
            } else if (nextAttackIndex_ == 3) {
                enemy_->SetState(EnemyState::Attack_Tackle);
            }
            nextAttackIndex_ = (nextAttackIndex_ + 1) % 4; // 次回のためにインクリメント
        }

    // 3. 攻撃中：アニメーションが完了したか監視
    } else {
        if (enemy_->GetAnimation()->HasFinishedAttack()) {
            enemy_->SetState(EnemyState::Idle);
            isWaitingForNextAttack_ = true;
            attackWaitTimer_ = kZeroThreshold;
            // 次の攻撃を順番にするためにフラグを更新（ここでは何もしない。開始時に更新済みのため）
            // または、安全のためここでも更新可能だが、Update内で開始時に計算済みなら不要。
            // ※今回は次ターンの保証のために上でインクリメントしているのでここは削除します。
        }
    }
}