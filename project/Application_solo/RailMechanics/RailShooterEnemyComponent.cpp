#include "RailMechanics/RailShooterEnemyComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Player/TargetableComponent.h"
#include "Player/PlayerHealthComponent.h"
#include "Combat/EnemyBulletComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Framework/Scene/BaseScene.h"
#include "Physics/CollisionManager.h"
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
}

void RailShooterEnemyComponent::Initialize() {
    hp_ = 100;
    isActive_ = true;
    state_ = EnemyAIState::Approach;
    stateTimer_ = 0.0f;
    shootTimer_ = 0.6f;
    hoverTimer_ = 0.0f;

    auto targetable = gameObject_->GetComponent<TargetableComponent>();
    if (!targetable) {
        targetable = gameObject_->AddComponent<TargetableComponent>().get();
        targetable->Initialize();
    }
    if (targetable) {
        targetable->SetTargetablePredicate([this]() { return IsAlive(); });
    }

    if (gameObject_) {
        auto collider = gameObject_->GetComponent<SphereColliderComponent>();
        if (!collider) {
            collider = gameObject_->AddComponent<SphereColliderComponent>().get();
            collider->Initialize();
        }
        if (collider) {
            collider->isTrigger_ = true;
            collider->SetLocalRadius(2.0f);

            auto cm = BaseModel::GetIrufemiEngine()->GetCollisionManager();
            if (cm) {
                collider->layer_ = cm->GetLayerMask("Enemy");
                collider->mask_ = cm->GetLayerMask("Player") | cm->GetLayerMask("Debris_Player");
            }
        }
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

    float dt = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (dt <= 0.0f) {
        return;
    }

    auto transform = GetTransform();
    if (!transform) {
        return;
    }

    auto playerObj = GetPlayerObject();
    Irufemi::Vector3 myPos = transform->GetWorldPosition();

    if (!playerObj) {
        // プレイヤー不在時のフォールバック直進
        myPos.z -= speed_ * dt;
        transform->SetWorldPosition(myPos);
        return;
    }

    Irufemi::Vector3 playerPos = playerObj->GetTransform()->GetWorldPosition();
    float targetZ = playerPos.z + targetDistance_;

    switch (state_) {
    case EnemyAIState::Approach: {
        // 自機前方定位置 (targetDistance_) へスムーズに接近
        float diffZ = targetZ - myPos.z;
        if (std::abs(diffZ) > 1.0f) {
            float moveDir = (diffZ > 0.0f) ? 1.0f : -1.0f;
            myPos.z += moveDir * speed_ * 1.5f * dt;
            if ((moveDir > 0.0f && myPos.z > targetZ) || (moveDir < 0.0f && myPos.z < targetZ)) {
                myPos.z = targetZ;
            }
        } else {
            myPos.z = targetZ;
        }

        // X, Yも緩やかにプレイヤー正面近傍へ収束
        myPos.x += (playerPos.x - myPos.x) * 2.0f * dt;
        myPos.y += ((playerPos.y + 2.0f) - myPos.y) * 2.0f * dt;

        stateTimer_ += dt;
        if (std::abs(myPos.z - targetZ) < 3.0f || stateTimer_ >= 3.0f) {
            state_ = EnemyAIState::Combat;
            stateTimer_ = 0.0f;
            shootTimer_ = 0.6f; // 初弾タイマー
        }
        break;
    }
    case EnemyAIState::Combat: {
        // 自機の前進に同期して前方一定距離を維持
        myPos.z = targetZ;

        // X, Y方向にゆらゆらと浮遊運動
        hoverTimer_ += dt;
        myPos.x += std::sin(hoverTimer_ * 2.5f) * 4.0f * dt;
        myPos.y += std::cos(hoverTimer_ * 2.0f) * 2.5f * dt;

        // 自機狙い弾の射撃
        shootTimer_ -= dt;
        if (shootTimer_ <= 0.0f) {
            ShootAtPlayer(playerPos);
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
        myPos.z -= speed_ * 2.5f * dt;

        // 自機後方に完全に抜けたら消滅
        if (myPos.z < playerPos.z - 20.0f) {
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

    transform->SetWorldPosition(myPos);
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
    Irufemi::Vector3 dir = {
        playerPos.x - myPos.x,
        playerPos.y - myPos.y,
        playerPos.z - myPos.z
    };
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len > 0.0001f) {
        dir.x /= len;
        dir.y /= len;
        dir.z /= len;
    } else {
        dir = { 0.0f, 0.0f, -1.0f };
    }

    auto bulletObj = std::make_shared<GameObject>("EnemyBullet");
    bulletObj->SetIsSerializable(false);

    auto bulletTrans = bulletObj->GetTransform();
    bulletTrans->SetWorldPosition(myPos);
    bulletTrans->SetScale({0.8f, 0.8f, 0.8f});

    auto meshRenderer = bulletObj->AddComponent<MeshRendererComponent>();
    meshRenderer->LoadModel("resources/model/BossBulletSphere.obj");
    meshRenderer->Initialize();

    auto collider = bulletObj->AddComponent<SphereColliderComponent>();
    collider->Initialize();
    collider->isTrigger_ = true;
    collider->SetLocalRadius(0.8f);

    auto cm = BaseModel::GetIrufemiEngine()->GetCollisionManager();
    if (cm) {
        collider->layer_ = cm->GetLayerMask("Enemy");
        collider->mask_ = cm->GetLayerMask("Player");
    }

    auto bulletComp = bulletObj->AddComponent<EnemyBulletComponent>();
    bulletComp->Initialize();
    bulletComp->Launch(dir, 32.0f, 10);

    scene->AddGameObject(bulletObj);
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
