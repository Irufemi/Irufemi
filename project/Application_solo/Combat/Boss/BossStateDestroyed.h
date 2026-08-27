#pragma once
#include "Combat/Boss/IBossState.h"

class BossStateDestroyed : public IBossState {
public:
    void Enter(BossComponent* boss) override;
    void Update(BossComponent* boss) override;
    void Exit(BossComponent* boss) override;
    void OnTakeDamage(BossComponent* boss, float damage) override;

private:
    class CameraShakeComponent* shakeComp_ = nullptr;
    bool hasFinished_ = false;
};
