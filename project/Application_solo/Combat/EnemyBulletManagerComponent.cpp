#include "Combat/EnemyBulletManagerComponent.h"
#include "Combat/EnemyBulletComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Physics/CollisionManager.h"
#include "Core/System/IrufemiEngine.h"

EnemyBulletManagerComponent::EnemyBulletManagerComponent() = default;

void EnemyBulletManagerComponent::Initialize() {
    WarmupPool();
}

void EnemyBulletManagerComponent::Start() {
    WarmupPool();
}

void EnemyBulletManagerComponent::OnDestroy() {
    activeBulletHandles_.clear();
    bulletPool_.reset();
}

EnemyBulletManagerComponent* EnemyBulletManagerComponent::GetOrCreate(BaseScene* scene) {
    if (!scene) {
        return nullptr;
    }

    if (auto obj = scene->FindGameObject("EnemyBulletManager")) {
        if (auto mgr = obj->GetComponent<EnemyBulletManagerComponent>()) {
            return mgr;
        }
    }

    // シーン上に存在しない場合は自動プロビジョニング
    auto managerObj = std::make_shared<GameObject>("EnemyBulletManager");
    managerObj->SetIsSerializable(false);
    auto mgr = managerObj->AddComponent<EnemyBulletManagerComponent>();
    scene->AddGameObject(managerObj);
    mgr->Initialize();
    return mgr.get();
}

void EnemyBulletManagerComponent::WarmupPool() {
    if (isWarmedUp_ || !gameObject_) {
        return;
    }

    auto scene = gameObject_->GetScene();
    if (!scene) {
        return;
    }

    bulletPool_ = std::make_unique<ObjectPool<GameObject>>(maxBullets_, [this, scene]() {
        auto bullet = std::make_shared<GameObject>("EnemyBullet");
        bullet->SetIsSerializable(false);

        // レンダラー設定
        auto meshRenderer = bullet->AddComponent<MeshRendererComponent>();
        meshRenderer->LoadModel(bulletModelPath_);
        meshRenderer->Initialize();

        // コライダー設定
        auto collider = bullet->AddComponent<SphereColliderComponent>();
        collider->Initialize();
        collider->isTrigger_ = true;
        collider->SetLocalRadius(0.4f);

        if (auto engine = GetEngine()) {
            if (auto cm = engine->GetCollisionManager()) {
                collider->layer_ = cm->GetLayerMask("Enemy");
                collider->mask_ = cm->GetLayerMask("Player");
            }
        }

        // 弾ロジック設定
        auto bulletComp = bullet->AddComponent<EnemyBulletComponent>();
        bulletComp->Initialize();
        bulletComp->SetManager(this);

        // シーンに登録し、非アクティブにして待機
        scene->AddGameObject(bullet);
        bullet->SetIsActive(false);

        return bullet;
    });

    isWarmedUp_ = true;
}

GameObject* EnemyBulletManagerComponent::FireBullet(const Irufemi::Vector3& origin, const Irufemi::Vector3& direction, float speed, int damage, float scale) {
    if (!isWarmedUp_) {
        WarmupPool();
    }
    if (!bulletPool_) {
        return nullptr;
    }

    auto handle = bulletPool_->Acquire();
    if (!handle.IsValid()) {
        return nullptr; // プール枯渇時は安全にスキップ
    }

    auto bullet = bulletPool_->Resolve(handle);
    if (!bullet) {
        return nullptr;
    }

    // トランスフォームとコライダーの設定
    if (auto transform = bullet->GetTransform()) {
        transform->SetWorldPosition(origin);
        transform->SetScale({scale, scale, scale});
    }

    if (auto collider = bullet->GetComponent<SphereColliderComponent>()) {
        collider->SetLocalRadius(scale);
    }

    if (auto bulletComp = bullet->GetComponent<EnemyBulletComponent>()) {
        bulletComp->SetManager(this);
        bulletComp->Launch(direction, speed, damage);
    }

    bullet->SetIsActive(true);
    activeBulletHandles_[bullet.get()] = handle;

    return bullet.get();
}

void EnemyBulletManagerComponent::ReturnBullet(GameObject* bullet) {
    if (!bullet || !bulletPool_) {
        return;
    }

    bullet->SetIsActive(false);

    auto it = activeBulletHandles_.find(bullet);
    if (it != activeBulletHandles_.end()) {
        bulletPool_->Release(it->second);
        activeBulletHandles_.erase(it);
    }
}
