#include "IEnemy.h"
#include "contents/player/Player.h"

void IEnemy::Initialize(const Vector3& position) {
    transform_.translate = position;
    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    isDead_ = false;
}

void IEnemy::OnCollision(Player* player) {
    // プレイヤーがダッシュ中なら何もしない
    if (player->IsDashing()) {
        return;
    }
    // 死亡処理
    isDead_ = true;
}

AABB IEnemy::GetAABB() const {
    AABB aabb;
    aabb.min = { transform_.translate.x - width_ / 2.0f, transform_.translate.y - height_ / 2.0f, transform_.translate.z - width_ / 2.0f };
    aabb.max = { transform_.translate.x + width_ / 2.0f, transform_.translate.y + height_ / 2.0f, transform_.translate.z + width_ / 2.0f };
    return aabb;
}