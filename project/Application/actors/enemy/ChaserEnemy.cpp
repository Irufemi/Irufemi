#include "ChaserEnemy.h"
#include "../player/Player.h"
#include "function/Math.h"

ChaserEnemy::ChaserEnemy() : Enemy() {}

ChaserEnemy::~ChaserEnemy() {}

const char* ChaserEnemy::GetModelFile() const {
    return "TD_ChaserEnemy.obj"; // プロジェクトにモデルがなければ Enemy のデフォルトが使われる
}

void ChaserEnemy::Update(const std::list<Wall*>& walls, const std::list<HealerActor*>& healers) {
 
    if (!player_) {
        Enemy::Update(walls, healers);
        return;
    }

 
    Enemy::Update(walls, healers);

   
    Vector3 playerPos = player_->GetPosition();
    Vector3 direction = playerPos - transform_.translate;
    float dist = Math::Length(direction);
    if (dist > 1e-4f) {
        Vector3 dirN = Math::Normalize(direction);
        float chaseSpeed = 0.12f; 
        transform_.translate += dirN * chaseSpeed;
    }

    UpdateOBB();
}
