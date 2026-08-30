#pragma once
#include "Combat/Boss/IBossState.h"

class BossStateCoreExposed : public IBossState {
public:
    void Enter(BossComponent* boss) override;
    void Update(BossComponent* boss) override;
    void Exit(BossComponent* boss) override;
    void OnTakeDamage(BossComponent* boss, float damage) override;

    bool IsCoreExposed() const override {
        return true;
    }
};
