#include "PlayerStatus.h"
#include "Engine/Core/Math/Geometry/Math.h"
#include "Framework/SceneManager.h" 
#include "../enemy/Enemy.h"
#include <cmath>

void PlayerStatus::Initialize() {
    hp_ = kMaxHp;
    isDead_ = false;
    invincibleTimer_ = 0;

    knockbackTarget_ = nullptr;
    knockbackVelocity_ = { 0.0f, 0.0f, 0.0f };
    knockbackTimer_ = 0;
}

void PlayerStatus::Update() {
    // 無敵時間タイマーの減算
    if (invincibleTimer_ > 0) {
        invincibleTimer_--;
    }
}

void PlayerStatus::UpdateKnockback() {
    // 敵を吹き飛ばす（ノックバック）処理
    if (knockbackTarget_ && knockbackTimer_ > 0) {
        Transform& enemyTransform = knockbackTarget_->GetGlobalTransform();

        enemyTransform.translate.x += knockbackVelocity_.x;
        enemyTransform.translate.y += knockbackVelocity_.y;
        enemyTransform.translate.z += knockbackVelocity_.z;

        // 摩擦（徐々に減速させる）
        knockbackVelocity_.x *= 0.85f;
        knockbackVelocity_.y *= 0.85f;
        knockbackVelocity_.z *= 0.85f;

        knockbackTimer_--;
        if (knockbackTimer_ <= 0) {
            knockbackTarget_ = nullptr;
        }
    }
}

void PlayerStatus::ApplyDamage(int damage, bool isCharging, IrufemiEngine* engine) {
    // 死亡時や無敵中はダメージを受けない
    if (isDead_ || invincibleTimer_ > 0) return;

    // チャージ中ならダメージ2倍
    int finalDamage = isCharging ? damage * 2 : damage;
    hp_ -= finalDamage;

    if (hp_ <= 0) {
        hp_ = 0;
        isDead_ = true;

        // 死亡時にゲームオーバーシーンへ遷移
        if (engine && engine->GetSceneManager()) {
            engine->GetSceneManager()->Request("GameOver");
        }
    } else {
        // ダメージを受けたら60フレーム（約1秒）無敵になる
        invincibleTimer_ = 60;
    }
}

void PlayerStatus::HitAndKnockback(Enemy* enemy, const Vector3& playerTranslate) {
    if (!enemy) return;

    knockbackTarget_ = enemy;
    knockbackTimer_ = 20;

    Vector3 pPos = playerTranslate;
    Vector3 ePos = enemy->GetGlobalTransform().translate;
    Vector3 dir = { ePos.x - pPos.x, 0.0f, ePos.z - pPos.z };

    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len > 0.001f) {
        float power = 2.0f; // 吹き飛ばす力
        knockbackVelocity_.x = (dir.x / len) * power;
        knockbackVelocity_.y = (dir.y / len) * power;
        knockbackVelocity_.z = (dir.z / len) * power;
    }
}

PlayerCollider PlayerStatus::GetCollider(const Vector3& playerTranslate, const Vector3& playerRotate, const Vector3& missileVibration) const {
    PlayerCollider col;

    // やられ判定にもミサイル発射時の振動を反映させる
    col.center = playerTranslate + missileVibration;
    col.center.y += 0.2f;
    col.radius = kColliderRadius;
    col.obb.center = col.center;

    Matrix4x4 rotateMatrix = Math::MakeRotateMatrix(Math::MakeRotateAxisAngleQuaternion({ 0.0f, 1.0f, 0.0f }, playerRotate.y));
    col.obb.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] };
    col.obb.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] };
    col.obb.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] };
    col.obb.size = { 0.3f, 0.3f, 0.3f };

    return col;
}