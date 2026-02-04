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

    // プレイヤーの移動状態を推定して、停止時間を蓄積
    static Vector3 lastPlayerPos = playerPos;
    static float playerIdleTime = 0.0f;
    const float kFrameDt = 1.0f / 60.0f;
    const float kIdleEpsilon = 1e-3f;
    const float kIdleThresholdSec = 1.5f; // 一定時間停止で加速

    float deltaLen = Math::Length(playerPos - lastPlayerPos);
    if (deltaLen <= kIdleEpsilon) {
        playerIdleTime += kFrameDt;
    } else {
        playerIdleTime = 0.0f;
    }
    lastPlayerPos = playerPos;

    if (dist > 1e-4f) {
        Vector3 dirN = Math::Normalize(direction);
        float chaseSpeed = 0.12f;
        // プレイヤーが一定時間止まっていて、かつ剣をチャージしていないなら加速
        if (playerIdleTime >= kIdleThresholdSec && !player_->IsCharging()) {
            chaseSpeed *= 1.8f; // 加速倍率（調整可）
        }
        transform_.translate += dirN * chaseSpeed;
    }

    UpdateOBB();
}
