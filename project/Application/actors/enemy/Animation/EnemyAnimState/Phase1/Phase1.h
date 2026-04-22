#pragma once
#include "../../IEnemyAnimationState.h"
#include "../../../EnemyState.h"
#include "Idle/Phase1_Idle.h"
#include "Beam/Phase1_Beam.h"
#include "Stomp/Phase1_Stomp.h"
#include "NeckAttack/Phase1_NeckAttack.h"
#include <map>
#include <memory>

class Phase1 : public IEnemyAnimationState {
public:
    Phase1();
    ~Phase1() override = default;

    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    bool IsFinished() const override;
    bool IsFiring() const override;

    // AIなどから内部の攻撃ステートを指定するための関数
    void ChangeState(EnemyState newState, Enemy* enemy);

private:
    IEnemyAnimationState* currentLocalState_ = nullptr;
    std::map<EnemyState, std::unique_ptr<IEnemyAnimationState>> localStateMap_;
};
