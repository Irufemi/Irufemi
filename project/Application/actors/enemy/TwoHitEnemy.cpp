#include "TwoHitEnemy.h"

#include "camera/Camera.h"
#include <chrono>

TwoHitEnemy::TwoHitEnemy() { remainingHits_ = 2; }
TwoHitEnemy::~TwoHitEnemy() = default;

void TwoHitEnemy::Initialize(Camera* camera, Vector3 pos) {
    
    Enemy::Initialize(camera, pos);
    remainingHits_ = 2;
    lastHitTime_ = std::chrono::steady_clock::time_point{};
    lastSlashId_ = 0;
}

void TwoHitEnemy::HitBySword() {
   
    static uint32_t fallbackId = 1;
    HitBySlash(fallbackId++);
}

void TwoHitEnemy::HitBySlash(uint32_t slashId) {
   
    if (slashId != 0 && lastSlashId_ == slashId) {
        return;
    }

    lastSlashId_ = slashId;

    using namespace std::chrono;
    auto now = steady_clock::now();
    if (lastHitTime_.time_since_epoch().count() != 0) {
        duration<float> diff = now - lastHitTime_;
        if (diff.count() < kHitCooldownSeconds) {
           
            return;
        }
    }

    lastHitTime_ = now;

    --remainingHits_;

    
    if (remainingHits_ == 1) {
        SetModelFile("TD_Enemy.obj");
        ReloadModel();
    }

    if (remainingHits_ <= 0) {
        Kill();
    }
}

void TwoHitEnemy::Kill() {
 
    if (remainingHits_ > 0) {
     
        return;
    }

   
    Enemy::Kill();
}

void TwoHitEnemy::Draw() {
    Enemy::Draw();
}

void TwoHitEnemy::Update(const std::list<Wall*>& walls, const std::list<HealerActor*>& healers)
{

    Enemy::Update(walls, healers);
}
