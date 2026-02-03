#include "TwoHitEnemy.h"

#include "camera/Camera.h"
#include "contents/wall/Wall.h"
#include <chrono>

TwoHitEnemy::TwoHitEnemy() { wallsDestroyedCount_ = 0; hitCount_ = 0; }
TwoHitEnemy::~TwoHitEnemy() = default;

void TwoHitEnemy::Initialize(Camera* camera, Vector3 pos) {
    
    Enemy::Initialize(camera, pos);
    wallsDestroyedCount_ = 0;
    hitCount_ = 0;
    lastSlashId_ = 0;
}

void TwoHitEnemy::HitBySword() {
  
    ++hitCount_;
    if (hitCount_ == 1) {
        SetModelFile("TD_Enemy.obj");
        ReloadModel();
    }
    // do not kill on sword hit; death is based on walls destroyed
}

void TwoHitEnemy::HitBySlash(uint32_t slashId) {
  
    if (slashId != 0 && lastSlashId_ == slashId) return;
    lastSlashId_ = slashId;

  
    ++hitCount_;
    if (hitCount_ == 1) {
        SetModelFile("TD_Enemy.obj");
        ReloadModel();
    }
}

void TwoHitEnemy::Kill() {
    if (!IsAlive()) return;
    Enemy::Kill();
}

void TwoHitEnemy::Draw() {
    Enemy::Draw();
}

void TwoHitEnemy::Update(const std::list<Wall*>& walls, const std::list<HealerActor*>& healers)
{
    Enemy::Update(walls, healers);
}

void TwoHitEnemy::OnWallDestroyed(const Wall* wall) {
  
    ++wallsDestroyedCount_;
    if (wallsDestroyedCount_ == 1) {
      
        SetModelFile("TD_Enemy.obj");
        ReloadModel();
    }
    if (wallsDestroyedCount_ >= 2) {
        Kill();
    }
}
