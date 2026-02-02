#pragma once

#include "Enemy.h"

// Enemy that requires two sword hits to die.
class TwoHitEnemy : public Enemy {
public:
    TwoHitEnemy();
    ~TwoHitEnemy() override;

    void Initialize(Camera* camera, Vector3 pos);

    // override HitBySword to require 2 hits
    void HitBySword() override;

protected:
    // Use the same model as default Enemy so it will be visible by default
    const char* GetModelFile() const override { return "TD_Enemy.obj"; }

private:
    int remainingHits_ = 2;
};
