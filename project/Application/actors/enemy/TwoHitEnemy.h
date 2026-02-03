#pragma once

#include "Enemy.h"
#include <string>

class Camera;

// Enemy that dies after destroying two walls (instead of two sword hits).
class TwoHitEnemy : public Enemy {
public:
    TwoHitEnemy();
    ~TwoHitEnemy() override;

    void Initialize(Camera* camera, Vector3 pos);

    void HitBySword() override;

    // HitBySlash is not declared virtual in base class, so do not use 'override'
    void HitBySlash(uint32_t slashId);

    void Kill() override;

    void SetModelFile(const char* file) { modelFile_ = file ? file : "TD_Enemy.obj"; }

    void Draw() override;

    // Update is not virtual in base class, so do not use 'override'
    void Update(const std::list<class Wall*>& walls, const std::list<class HealerActor*>& healers);

    // Notified when a wall is destroyed; this enemy counts how many walls it has destroyed.
    void OnWallDestroyed(const Wall* wall) override;

protected:
    const char* GetModelFile() const override { return modelFile_.c_str(); }

    void OnRespawn() override { wallsDestroyedCount_ = 0; hitCount_ = 0; SetModelFile("TD_HardEnemy.obj"); }

private:
    int wallsDestroyedCount_ = 0; // number of walls this enemy has destroyed
    int hitCount_ = 0; // number of times hit by sword
    std::string modelFile_ = "TD_HardEnemy.obj";

    // keep a slash-id tracker to avoid duplicate slash handling if desired
    uint32_t lastSlashId_ = 0;
};
