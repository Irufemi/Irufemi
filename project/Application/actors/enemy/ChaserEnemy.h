#pragma once

#include "Enemy.h"

class ChaserEnemy : public Enemy {
public:
    ChaserEnemy();
    ~ChaserEnemy() override;

protected:
    // Use a distinct model file for chaser; fallback to existing if file not present
    const char* GetModelFile() const override;
};
