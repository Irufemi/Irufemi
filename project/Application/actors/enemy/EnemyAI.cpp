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

    if (enemy_->GetState() == EnemyState::Idle) {
        // 15秒周期のうち、5秒経過したら攻撃開始！
        if (std::fmod(timer_, stateChangeInterval_) > attackStartTime_) {
            enemy_->SetState(EnemyState::Attack_Beam);
        }
    }
}