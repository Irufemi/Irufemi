#pragma once

#include "Enemy.h"

class ChaserEnemy : public Enemy {
public:
    ChaserEnemy();
    ~ChaserEnemy() override;

  
    void Update(const std::list<class Wall*>& walls, const std::list<class HealerActor*>& healers) override;

protected:
  
    const char* GetModelFile() const override;
};
