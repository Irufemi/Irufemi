#include "RailMechanics/RailShooterEnemyComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/Component/Utility/SplineComponent.h"
#include "RailMechanics/SplineFollowerComponent.h"
#include "Player/TargetableComponent.h"
#include "Player/PlayerHealthComponent.h"
#include "Combat/EnemyBulletComponent.h"
#include "Combat/EnemyBulletManagerComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Framework/Scene/BaseScene.h"
#include "Physics/CollisionManager.h"
#include "Core/Math/MathFunction.h"
#include <cmath>
#include <algorithm>

void RailShooterEnemyComponent::OnRegisterProperties() {
    RegisterProperty("SpawnProgress", &spawnProgress_);
    RegisterProperty("Speed", &speed_);
    RegisterProperty("HP", &hp_);
    RegisterProperty("CombatDuration", &combatDuration_);
    RegisterProperty("ShootInterval", &shootInterval_);
    RegisterProperty("TargetDistance", &targetDistance_);
    RegisterProperty("BodyDamage", &bodyDamage_);
    RegisterProperty("BulletScale", &bulletScale_);
    RegisterProperty("BulletSpeed", &bulletSpeed_);
    RegisterProperty("CurrentDistanceOffset", &currentDistanceOffset_);
    RegisterProperty("BaseFormationOffsetX", &baseFormationOffset_.x);
    RegisterProperty("BaseFormationOffsetY", &baseFormationOffset_.y);
}

void RailShooterEnemyComponent::Initialize() {
    if (!gameObject_) {
        return;
    }
    hp_ = 100;
    isActive_ = true;
    state_ = EnemyAIState::Approach;
    stateTimer_ = 0.0f;
    shootTimer_ = 0.6f;
    hoverTimer_ = 0.0f;
    playerFollower_ = nullptr;
    cachedSpline_ = nullptr;
    baseFormationOffset_ = {0.0f, 0.0f};
    currentLocalOffset_ = {0.0f, 0.0f};
    currentDistanceOffset_ = targetDistance_ + 20.0f;

    auto targetable = gameObject_->GetComponent<TargetableComponent>();
    if (!targetable) {
        targetable = gameObject_->AddComponent<TargetableComponent>().get();
        targetable->Initialize();
    }
    if (targetable) {
        targetable->SetTargetablePredicate([this]() { return IsAlive(); });
    }

    // コライダーのサイズ・形状はプレハブ（アセット）側を100%尊重し、コードによる勝手な追加・上書きを行わない
    if (auto collider = gameObject_->GetComponent<SphereColliderComponent>()) {
        if (auto engine = GetEngine()) {
            if (auto cm = engine->GetCollisionManager()) {
                collider->layer_ = cm->GetLayerMask("Enemy");
                collider->mask_ = cm->GetLayerMask("Player") | cm->GetLayerMask("Debris_Player");
            }
        }
    }
}

void RailShooterEnemyComponent::SetRailTrackingParams(SplineComponent* spline, SplineFollowerComponent* follower,
                                                      float initialDistOffset, float targetDistOffset,
                                                      const Irufemi::Vector2& formationOffset) {
    cachedSpline_ = spline;
    playerFollower_ = follower;
    currentDistanceOffset_ = initialDistOffset;
    targetDistance_ = targetDistOffset;
    baseFormationOffset_ = formationOffset;
    currentLocalOffset_ = formationOffset;
}

void RailShooterEnemyComponent::Start() {
    if (auto scene = gameObject_ ? gameObject_->GetScene() : nullptr) {
        bulletManager_ = EnemyBulletManagerComponent::GetOrCreate(scene);
    }
}

GameObject* RailShooterEnemyComponent::GetPlayerObject() {
    if (!gameObject_) {
        return nullptr;
    }
    auto scene = gameObject_->GetScene();
    if (!scene) {
        return nullptr;
    }
    auto player = scene->FindGameObject("Player");
    if (!player) {
        player = scene->FindGameObject("PlayerCart");
    }
    return player.get();
}

void RailShooterEnemyComponent::Update() {
    if (!gameObject_ || !isActive_) {
        return;
    }

    float dt = GetEngine() ? GetEngine()->GetGameDeltaTime() : 0.0f;
    if (dt <= 0.0f) {
        return;
    }

    auto transform = GetTransform();
    if (!transform) {
        return;
    }

    // スプライン追従情報が未解決の場合はプレイヤーから自動解決
    if (!playerFollower_ || !cachedSpline_) {
        auto playerObj = GetPlayerObject();
        if (playerObj) {
            if (!playerFollower_) {
                playerFollower_ = playerObj->GetComponent<SplineFollowerComponent>();
            }
            if (playerFollower_ && !cachedSpline_) {
                cachedSpline_ = playerFollower_->GetCachedPath();
            }
        }
    }

    // フォールバック（スプラインが無い場合のみ直線前進）
    if (!playerFollower_ || !cachedSpline_) {
        Irufemi::Vector3 myPos = transform->GetWorldPosition();
        myPos.z -= speed_ * dt;
        transform->SetWorldPosition(myPos);
        return;
    }

    float playerDist = playerFollower_->GetCurrentDistance();

    switch (state_) {
    case EnemyAIState::Approach: {
        // 初期距離オフセットから目標交戦距離 (targetDistance_) へスムーズにレール上を接近
        float diffDist = targetDistance_ - currentDistanceOffset_;
        if (std::abs(diffDist) > 0.5f) {
            float moveDir = (diffDist > 0.0f) ? 1.0f : -1.0f;
            currentDistanceOffset_ += moveDir * speed_ * 1.5f * dt;
            if ((moveDir > 0.0f && currentDistanceOffset_ > targetDistance_) ||
                (moveDir < 0.0f && currentDistanceOffset_ < targetDistance_)) {
                currentDistanceOffset_ = targetDistance_;
            }
        } else {
            currentDistanceOffset_ = targetDistance_;
        }

        currentLocalOffset_ = baseFormationOffset_;

        stateTimer_ += dt;
        if (std::abs(currentDistanceOffset_ - targetDistance_) < 2.0f || stateTimer_ >= 3.0f) {
            state_ = EnemyAIState::Combat;
            stateTimer_ = 0.0f;
            shootTimer_ = 0.6f; // 初弾タイマー
        }
        break;
    }
    case EnemyAIState::Combat: {
        // 自機と等速で前方一定距離を完全維持
        currentDistanceOffset_ = targetDistance_;

        // レール局所断面での浮遊運動（サイン・コサイン波）
        hoverTimer_ += dt;
        currentLocalOffset_.x = baseFormationOffset_.x + std::sin(hoverTimer_ * 2.5f) * 4.0f;
        currentLocalOffset_.y = baseFormationOffset_.y + std::cos(hoverTimer_ * 2.0f) * 2.5f;

        // 自機狙い弾の射撃
        shootTimer_ -= dt;
        if (shootTimer_ <= 0.0f) {
            auto playerObj = GetPlayerObject();
            if (playerObj && playerObj->GetTransform()) {
                ShootAtPlayer(playerObj->GetTransform()->GetWorldPosition());
            }
            shootTimer_ = shootInterval_;
        }

        // 一定時間経過で離脱フェーズへ移行
        stateTimer_ += dt;
        if (stateTimer_ >= combatDuration_) {
            state_ = EnemyAIState::Disengage;
            stateTimer_ = 0.0f;
        }
        break;
    }
    case EnemyAIState::Disengage: {
        // 相対同期を解除し、自機の脇をすり抜けて後方へ急加速
        currentDistanceOffset_ -= speed_ * 2.5f * dt;

        // 自機後方に完全に抜けたら消滅（画面外へ抜けるまで安全に生存）
        if (currentDistanceOffset_ < -30.0f) {
            if (onDeathCallback_) {
                onDeathCallback_(gameObject_);
            } else {
                gameObject_->SetIsActive(false);
                gameObject_->Destroy();
            }
            return;
        }
        break;
    }
    }

    // レールスプライン上の位置と局所直交基底（Frenet-Serret）を計算
    float enemyRailDist = playerDist + currentDistanceOffset_;
    float totalLength = cachedSpline_->GetTotalLength();
    if (enemyRailDist < 0.0f) {
        enemyRailDist = 0.0f;
    }
    if (totalLength > 0.0f && enemyRailDist > totalLength) {
        enemyRailDist = totalLength;
    }

    Irufemi::Vector3 railCenter = cachedSpline_->GetPointAtDistance(enemyRailDist);
    Irufemi::Vector3 tangent = cachedSpline_->GetTangentAtDistance(enemyRailDist);

    // 局所基底の算出（Right, Up）
    Irufemi::Vector3 upWorld = {0.0f, 1.0f, 0.0f};
    Irufemi::Vector3 right = Irufemi::Math::Cross(upWorld, tangent);
    float lenR = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
    if (lenR > 0.0001f) {
        right.x /= lenR;
        right.y /= lenR;
        right.z /= lenR;
    } else {
        right = {1.0f, 0.0f, 0.0f};
    }
    Irufemi::Vector3 up = Irufemi::Math::Normalize(Irufemi::Math::Cross(tangent, right));

    // 最終ワールド座標の決定
    Irufemi::Vector3 finalPos = railCenter;
    finalPos.x += right.x * currentLocalOffset_.x + up.x * currentLocalOffset_.y;
    finalPos.y += right.y * currentLocalOffset_.x + up.y * currentLocalOffset_.y;
    finalPos.z += right.z * currentLocalOffset_.x + up.z * currentLocalOffset_.y;
    transform->SetWorldPosition(finalPos);

    // 自機と対面（-tangent）する姿勢制御
    Irufemi::Vector3 lookDir = {-tangent.x, -tangent.y, -tangent.z};
    float yaw = std::atan2(lookDir.x, lookDir.z);
    float pitch = std::asin(std::clamp(-lookDir.y, -1.0f, 1.0f));
    transform->SetWorldRotation(Irufemi::Vector3{pitch, yaw, 0.0f});
}

void RailShooterEnemyComponent::ShootAtPlayer(const Irufemi::Vector3& playerPos) {
    if (!gameObject_) {
        return;
    }
    auto scene = gameObject_->GetScene();
    if (!scene) {
        return;
    }
    auto transform = GetTransform();
    if (!transform) {
        return;
    }

    Irufemi::Vector3 myPos = transform->GetWorldPosition();
    Irufemi::Vector3 dir = {playerPos.x - myPos.x, playerPos.y - myPos.y, playerPos.z - myPos.z};
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len > 0.0001f) {
        dir.x /= len;
        dir.y /= len;
        dir.z /= len;
    } else {
        dir = {0.0f, 0.0f, -1.0f};
    }

    if (!bulletManager_) {
        bulletManager_ = EnemyBulletManagerComponent::GetOrCreate(scene);
    }
    if (bulletManager_) {
        bulletManager_->FireBullet(myPos, dir, bulletSpeed_, 10, bulletScale_);
    }
}

void RailShooterEnemyComponent::OnCollisionEnter(GameObject* other) {
    if (!other || !gameObject_) {
        return;
    }

    // プレイヤー本体との接触時（体当たり）
    if (auto health = other->GetComponent<PlayerHealthComponent>()) {
        if (!health->IsInvincible()) {
            health->TakeDamage(bodyDamage_);
        }
        // 体当たり後は敵自身も自爆・撃破
        TakeDamage(hp_);
    }
}

void RailShooterEnemyComponent::TakeDamage(float damage) {
    if (damage <= 0.0f) {
        return;
    }
    int intDamage = static_cast<int>(std::round(damage));
    if (intDamage < 1) {
        intDamage = 1;
    }
    TakeDamage(intDamage);
}

void RailShooterEnemyComponent::TakeDamage(int damage) {
    if (!IsAlive()) {
        return;
    }
    if (!gameObject_ || gameObject_->IsDestroyed()) {
        return;
    }

    hp_ -= damage;
    if (hp_ <= 0) {
        hp_ = 0;
        isActive_ = false;
        if (gameObject_) {
            auto callback = onDeathCallback_;
            if (callback) {
                callback(gameObject_);
            } else {
                gameObject_->SetIsActive(false);
                gameObject_->Destroy();
            }
        }
    }
}
