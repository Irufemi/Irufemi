#pragma once

#include "Enemy.h"
#include <string>
#include <chrono>

class Camera;

// Enemy that requires two sword hits to die.
class TwoHitEnemy : public Enemy {
public:
    TwoHitEnemy();
    ~TwoHitEnemy() override;

    void Initialize(Camera* camera, Vector3 pos);

  
    void HitBySword() override;

  
    void HitBySlash(uint32_t slashId);

   
    void Kill() override;
 
    void SetModelFile(const char* file) { modelFile_ = file ? file : "TD_Enemy.obj"; }

   
    void Draw() override;

  
    void Update(const std::list<class Wall*>& walls, const std::list<class HealerActor*>& healers);

protected:
  
    const char* GetModelFile() const override { return modelFile_.c_str(); }

private:
    int remainingHits_ = 2;
    std::string modelFile_ = "TD_HardEnemy.obj";

   
    std::chrono::steady_clock::time_point lastHitTime_ = std::chrono::steady_clock::time_point{};
    static inline constexpr float kHitCooldownSeconds = 0.25f; 

   
    uint32_t lastSlashId_ = 0;
};
