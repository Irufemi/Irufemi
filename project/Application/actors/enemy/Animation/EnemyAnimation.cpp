#include "EnemyAnimation.h"
#include "IEnemyAnimationState.h"
#include "EnemyAnimState/Phase1/Idle/Idle.h"
#include "EnemyAnimState/Phase1/Beam/Beam.h"
#include "EnemyAnimState/Phase1/Stomp/Stomp.h"
#include "EnemyAnimState/Phase2/Bite/Bite.h"
#include "EnemyAnimState/Phase2/Phase2.h"
#include "Enemy.h"

// コンストラクタ
EnemyAnimation::EnemyAnimation() = default;

// デストラクタ
EnemyAnimation::~EnemyAnimation() = default;

void EnemyAnimation::Initialize(Enemy* enemy) {
    enemy_ = enemy;
    stateMap_[EnemyState::Idle] = std::make_unique<Idle>();
    stateMap_[EnemyState::Attack_Beam] = std::make_unique<Beam>();
    stateMap_[EnemyState::Attack_Stomp] = std::make_unique<Stomp>();
    stateMap_[EnemyState::Attack_Bite] = std::make_unique<Bite>();
    stateMap_[EnemyState::Phase2] = std::make_unique<Phase2>();

    ChangeState(EnemyState::Idle);
}

void EnemyAnimation::Update(Player* player, float deltaTime) {
    if (currentState_) {
        currentState_->Update(enemy_, player, deltaTime);
    }
}

void EnemyAnimation::ChangeState(EnemyState newState) {
    auto it = stateMap_.find(newState);
    if (it == stateMap_.end()) return;

    if (currentState_) currentState_->Exit(enemy_);
    currentState_ = it->second.get();
    if (currentState_) currentState_->Enter(enemy_);
}

bool EnemyAnimation::IsFiring() const {
    return currentState_ ? currentState_->IsFiring() : false;
}

bool EnemyAnimation::HasFinishedAttack() const {
    return currentState_ ? currentState_->IsFinished() : false;
}

void EnemyAnimation::ResetAttackFinished() {
    // ステート側で管理されるため空
}