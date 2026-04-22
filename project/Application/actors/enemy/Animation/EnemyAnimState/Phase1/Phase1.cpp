#include "Phase1.h"

Phase1::Phase1() {
    localStateMap_[EnemyState::Idle] = std::make_unique<Phase1_Idle>();
    localStateMap_[EnemyState::Attack_Beam] = std::make_unique<Phase1_Beam>();
    localStateMap_[EnemyState::Attack_Stomp] = std::make_unique<Phase1_Stomp>();
    localStateMap_[EnemyState::Attack_Neck] = std::make_unique<Phase1_NeckAttack>();
}

void Phase1::Enter(Enemy* enemy) {
    // 最初にPhase1に入った時は、Idleステートにする
    ChangeState(EnemyState::Idle, enemy);
}

void Phase1::Update(Enemy* enemy, Player* player, float deltaTime) {
    if (currentLocalState_) {
        currentLocalState_->Update(enemy, player, deltaTime);
    }
}

void Phase1::Exit(Enemy* enemy) {
    if (currentLocalState_) {
        currentLocalState_->Exit(enemy);
        currentLocalState_ = nullptr;
    }
}

bool Phase1::IsFinished() const {
    if (currentLocalState_) {
        return currentLocalState_->IsFinished();
    }
    return false;
}

bool Phase1::IsFiring() const {
    if (currentLocalState_) {
        return currentLocalState_->IsFiring();
    }
    return false;
}

void Phase1::ChangeState(EnemyState newState, Enemy* enemy) {
    auto it = localStateMap_.find(newState);
    if (it == localStateMap_.end()) return;

    if (currentLocalState_) {
        currentLocalState_->Exit(enemy);
    }
    
    currentLocalState_ = it->second.get();
    
    if (currentLocalState_) {
        currentLocalState_->Enter(enemy);
    }
}
