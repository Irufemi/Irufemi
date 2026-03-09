#include "EnemyAI.h"
#include "Enemy.h"
#include <cmath>

EnemyAI::~EnemyAI() {}

void EnemyAI::Initialize(Enemy* enemy) {
    enemy_ = enemy;
    timer_ = 0.0f;
}

void EnemyAI::Update() {
    if (!enemy_) return;

    timer_ += 1.0f / 60.0f;

    // 全体のトランスフォームを取得（参照渡しで直接編集）
    Transform& globalTransform = enemy_->GetGlobalTransform();

    // -- 全体の動き制御 --
    if (enemy_->GetState() == EnemyState::Idle) {
        globalTransform.rotate.y += 0.005f; // 全体がY軸で回転する
    }

    // -- 状態の制御テスト --
    // 5秒ごとに待機と攻撃を切り替える例
    if (std::fmod(timer_, 10.0f) > 5.0f) {
        enemy_->SetState(EnemyState::Attack);
    } else {
        enemy_->SetState(EnemyState::Idle);
    }
}