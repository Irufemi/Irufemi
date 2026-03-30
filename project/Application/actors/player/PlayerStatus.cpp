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
        knockbackVelocity_.x *= kKnockbackFriction;
        knockbackVelocity_.y *= kKnockbackFriction;
        knockbackVelocity_.z *= kKnockbackFriction;

        knockbackTimer_--;
        if (knockbackTimer_ <= 0) {
            knockbackTarget_ = nullptr;
        }
    }
}

void PlayerStatus::ApplyDamage(int damage, bool isCharging, IrufemiEngine* engine) {
    if (isDead_ || invincibleTimer_ > 0) return;

    if (isCharging) {
        hp_ -= damage / 2;
    } else {
        hp_ -= damage;
    }

    // ★修正：定数を使用して被ダメージ時の無敵時間を付与
    invincibleTimer_ = kInvincibleFramesOnDamage;

    if (hp_ <= 0) {
        hp_ = 0;
        isDead_ = true;

        if (engine) {
            engine->GetSceneManager()->Request("GameOver");
        }
    }
}

void PlayerStatus::HitAndKnockback(Enemy* enemy, const Vector3& playerTranslate) {
    if (!enemy) return;

    knockbackTarget_ = enemy;
    knockbackTimer_ = kKnockbackDuration;

    Vector3 pPos = playerTranslate;
    Vector3 ePos = enemy->GetGlobalTransform().translate;
    Vector3 dir = { ePos.x - pPos.x, 0.0f, ePos.z - pPos.z };

    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len > kKnockbackMinDistance) {
        knockbackVelocity_.x = (dir.x / len) * kKnockbackPower;
        knockbackVelocity_.y = (dir.y / len) * kKnockbackPower;
        knockbackVelocity_.z = (dir.z / len) * kKnockbackPower;
    }
}

PlayerCollider PlayerStatus::GetCollider(const Vector3& playerTranslate, const Vector3& playerRotate, const Vector3& missileVibration) const {
    PlayerCollider col;

    // やられ判定にもミサイル発射時の振動を反映させる
    col.center = playerTranslate + missileVibration;
    // ★修正：定数を使用してやられ判定のYオフセットを決定
    col.center.y += kColliderOffsetY;
    col.radius = kColliderRadius;
    col.obb.center = col.center;

    // Y軸（ヨー角）の回転から直接OBBの各軸の方向ベクトルを計算（コンパイルエラー回避）
    float cosY = std::cos(playerRotate.y);
    float sinY = std::sin(playerRotate.y);

    // OBBのローカルX軸、Y軸、Z軸の向きを設定
    col.obb.orientations[0] = { cosY, 0.0f, -sinY }; // X軸
    col.obb.orientations[1] = { 0.0f, 1.0f, 0.0f };  // Y軸
    col.obb.orientations[2] = { sinY, 0.0f, cosY };  // Z軸

    // ★修正：定数を使用してOBBのサイズを指定
    col.obb.size = { kColliderObbSize, kColliderObbSize, kColliderObbSize };

    return col;
}