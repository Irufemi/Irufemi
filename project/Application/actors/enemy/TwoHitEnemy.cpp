#include "TwoHitEnemy.h"

#include "camera/Camera.h"

TwoHitEnemy::TwoHitEnemy() { remainingHits_ = 2; }
TwoHitEnemy::~TwoHitEnemy() = default;

void TwoHitEnemy::Initialize(Camera* camera, Vector3 pos) {
    // call base initialize
    Enemy::Initialize(camera, pos);
    remainingHits_ = 2;
}

void TwoHitEnemy::HitBySword() {
    --remainingHits_;
    if (remainingHits_ <= 0) {
        Kill();
    }
}

void TwoHitEnemy::Draw() {
   
    Enemy::Draw();
}
