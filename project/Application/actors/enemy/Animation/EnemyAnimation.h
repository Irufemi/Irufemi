#pragma once
#include "EnemyState.h"
#include <memory>
#include <map>

// 前方宣言
class Enemy;
class Player;
class IEnemyAnimationState;

class EnemyAnimation {
public:
    EnemyAnimation();
    ~EnemyAnimation();

    void Initialize(Enemy* enemy);
    void Update(Player* player, float deltaTime);
    void ChangeState(EnemyState newState);

    bool IsFiring() const;
    bool HasFinishedAttack() const;
    void ResetAttackFinished();

private:
    Enemy* enemy_ = nullptr;
    IEnemyAnimationState* currentState_ = nullptr;

    // unique_ptr を使うマップ。これの破棄に IEnemyAnimationState の定義が必要
    std::map<EnemyState, std::unique_ptr<IEnemyAnimationState>> stateMap_;
};