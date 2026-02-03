#pragma once

#include "Enemy.h"
#include <string>

class Camera;

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

 
    void OnWallDestroyed(const Wall* wall) override;

protected:
    const char* GetModelFile() const override { return modelFile_.c_str(); }

    void OnRespawn() override { wallsDestroyedCount_ = 0; hitCount_ = 0; SetModelFile("TD_HardEnemy.obj"); }

private:
    int wallsDestroyedCount_ = 0;
    int hitCount_ = 0; 
    std::string modelFile_ = "TD_HardEnemy.obj";

   
    uint32_t lastSlashId_ = 0;
};
