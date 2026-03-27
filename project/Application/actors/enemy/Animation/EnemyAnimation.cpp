#include "EnemyAnimation.h"
#include "IEnemyAnimationState.h"
#include "EnemyAnimState_Idle.h"
#include "EnemyAnimState_Beam.h"
#include "EnemyAnimState_Stomp.h"
#include "Enemy.h"

// コンストラクタ
EnemyAnimation::EnemyAnimation() = default;

// ★重要：ここで定義することで IEnemyAnimationState の中身が判明している状態で破棄が生成される
EnemyAnimation::~EnemyAnimation() = default;

void EnemyAnimation::Initialize(Enemy* enemy) {
    enemy_ = enemy;
    stateMap_[EnemyState::Idle] = std::make_unique<EnemyAnimState_Idle>();
    stateMap_[EnemyState::Attack_Beam] = std::make_unique<EnemyAnimState_Beam>();
    stateMap_[EnemyState::Attack_Stomp] = std::make_unique<EnemyAnimState_Stomp>();

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