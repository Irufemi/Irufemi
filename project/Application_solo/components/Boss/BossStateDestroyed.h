#pragma once
#include "IBossState.h"

class BossStateDestroyed : public IBossState {
public:
    void Enter(BossComponent* boss) override;
    void Update(BossComponent* boss) override;
    void Exit(BossComponent* boss) override;
    void OnTakeDamage(BossComponent* boss, float damage) override;
};
