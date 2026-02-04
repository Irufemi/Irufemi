#include "TwoHitEnemy.h"

#include "camera/Camera.h"
#include "contents/wall/Wall.h"
#include "3D/Effect/EffectSystem.h"
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

        if (effectSystem_) {
            Transform t;
            t.translate = transform_.translate;
            effectSystem_->Play(EffectType::kArmorBreak, t);
        }

        SetModelFile("TD_Enemy.obj");
        ReloadModel();
    }
    else if (hitCount_ >= 2) {
        Kill();
    }
   
}

void TwoHitEnemy::HitBySlash(uint32_t slashId) {
  
    if (slashId != 0 && lastSlashId_ == slashId) return;
    lastSlashId_ = slashId;

  
    ++hitCount_;
    if (hitCount_ == 1) {
        // デバッグ出力
        OutputDebugStringA("=== ArmorBreak Effect Called ===\n");

        if (effectSystem_) {
            OutputDebugStringA("effectSystem_ is valid\n");
            Transform t;
            t.translate = transform_.translate;

            char buf[128];
            sprintf_s(buf, "Position: %.2f, %.2f, %.2f\n", t.translate.x, t.translate.y, t.translate.z);
            OutputDebugStringA(buf);

            effectSystem_->Play(EffectType::kArmorBreak, t);
        } else {
            OutputDebugStringA("effectSystem_ is NULL!\n");
        }

        SetModelFile("TD_Enemy.obj");
        ReloadModel();
    }
    else if (hitCount_ >= 2) {
        Kill();
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

        if (effectSystem_) {
            Transform t;
            t.translate = transform_.translate;
            effectSystem_->Play(EffectType::kArmorBreak, t);
        }
      
        SetModelFile("TD_Enemy.obj");
        ReloadModel();
    
        if (hitCount_ < 1) hitCount_ = 1;
    }
    if (wallsDestroyedCount_ >= 2) {
        Kill();
    }
}
