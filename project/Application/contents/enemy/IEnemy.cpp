#include "IEnemy.h"
#include "contents/player/Player.h"

void IEnemy::Initialize(const Vector3& position) {
    transform_.translate = position;
    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    isDead_ = false;
}

void IEnemy::OnCollision(Player* player) {
    // プレイヤーが攻撃中でなければ何もしない
    if (!player->IsAttacking()) {
        return;
    }
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

// ワールド座標を取得
Vector3 IEnemy::GetWorldPosition() const {

    // ワールド座標を入れる変数
    Vector3 worldPos;
    // ワールド行列の平行移動成分を取得(ワールド座標)
    worldPos.x = worldMatrix_.m[3][0];
    worldPos.y = worldMatrix_.m[3][1];
    worldPos.z = worldMatrix_.m[3][2];

    return worldPos;
}