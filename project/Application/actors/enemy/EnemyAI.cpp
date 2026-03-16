#include "EnemyAI.h"
#include "Enemy.h"
#include <cmath>

EnemyAI::~EnemyAI() {}

void EnemyAI::Initialize(Enemy* enemy) {
    enemy_ = enemy;
    timer_ = 0.0f;
    attackWaitTimer_ = 0.0f;
    isWaitingForNextAttack_ = false;
}

void EnemyAI::Update() {
    if (!enemy_) return;

    // アニメーション側からビーム一連の動作が完了したか確認
    // ※Enemyクラスに GetAnimation() ゲッターがある前提です
    if (enemy_->GetAnimation()->HasFinishedAttack()) {
        // アニメーション側の完了フラグをリセット
        enemy_->GetAnimation()->ResetAttackFinished();

        // 待機モードへ移行
        isWaitingForNextAttack_ = true;
        attackWaitTimer_ = 0.0f;
    }

    // 次の攻撃までの待機処理
    if (isWaitingForNextAttack_) {
        attackWaitTimer_ += 1.0f / 60.0f;

        if (attackWaitTimer_ >= attackInterval_) {
            // 待機時間を過ぎたらビーム状態へ
            enemy_->SetState(EnemyState::Attack_Beam);
            isWaitingForNextAttack_ = false;
        }
    } else {
        // 初期状態（ゲーム開始時など）でIdleなら、最初の攻撃を開始させる
        if (enemy_->GetState() == EnemyState::Idle) {
            enemy_->SetState(EnemyState::Attack_Beam);
        }
    }
}