#pragma once

#include "Enemy.h"
#include <string>

class Camera;

// Enemy that requires two sword hits to die.
class TwoHitEnemy : public Enemy {
public:
    TwoHitEnemy();
    ~TwoHitEnemy() override;

    void Initialize(Camera* camera, Vector3 pos);

    // override HitBySword to require 2 hits
    void HitBySword() override;

    // allow changing the model file used by this enemy
    void SetModelFile(const char* file) { modelFile_ = file ? file : "TD_Enemy.obj"; }

    // ensure we can customize drawing if needed
    void Draw() override;

protected:
    // Use the same model as default Enemy so it will be visible by default
    const char* GetModelFile() const override { return modelFile_.c_str(); }

private:
    int remainingHits_ = 2;
    std::string modelFile_ = "TD_Enemy.obj";
};
