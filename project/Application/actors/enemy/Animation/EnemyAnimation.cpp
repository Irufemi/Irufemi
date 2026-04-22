#include "EnemyAnimation.h"
#include "IEnemyAnimationState.h"
#include "EnemyAnimState/Phase1/Phase1.h"
#include "EnemyAnimState/Phase2/Phase2.h"
#include "Enemy.h"

// コンストラクタ
EnemyAnimation::EnemyAnimation() = default;

// デストラクタ
EnemyAnimation::~EnemyAnimation() = default;

void EnemyAnimation::Initialize(Enemy* enemy) {
    enemy_ = enemy;
    stateMap_[EnemyState::Phase1] = std::make_unique<Phase1>();
    stateMap_[EnemyState::Phase2] = std::make_unique<Phase2>();

    // 初期状態はPhase1
    ChangeState(EnemyState::Phase1);
}

void EnemyAnimation::Update(Player* player, float deltaTime) {
    if (currentState_) {
        currentState_->Update(enemy_, player, deltaTime);
    }
}

void EnemyAnimation::ChangeState(EnemyState newState) {
    // 根本的なフェーズ切り替えの場合
    if (newState == EnemyState::Phase1 || newState == EnemyState::Phase2) {
        auto it = stateMap_.find(newState);
        if (it == stateMap_.end()) return;

        if (currentState_) currentState_->Exit(enemy_);
        currentState_ = it->second.get();
        if (currentState_) currentState_->Enter(enemy_);
    } else {
        // Attack_Neckなどの個別ステート切り替え指令が来た場合、現在がPhase1ならPhase1マネージャーに横流しする
        if (currentState_) {
            Phase1* phase1 = dynamic_cast<Phase1*>(currentState_);
            if (phase1) {
                phase1->ChangeState(newState, enemy_);
            }
        }
    }
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