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

    // 指定した周期内で、一定時間を超えたら攻撃状態へ移行
    if (std::fmod(timer_, stateChangeInterval_) > attackStartTime_) {
        enemy_->SetState(EnemyState::Attack_Beam);
    } else {
        enemy_->SetState(EnemyState::Idle);
    }
}