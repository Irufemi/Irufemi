#include "Combat/EnemyBulletComponent.h"
#include "Combat/EnemyBulletManagerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Player/PlayerHealthComponent.h"
#include "Effects/EffectManagerComponent.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Core/System/IrufemiEngine.h"
#include "Framework/Scene/BaseScene.h"

void EnemyBulletComponent::Initialize() {
    lifeTimer_ = 0.0f;
}

void EnemyBulletComponent::Launch(const Irufemi::Vector3& direction, float speed, int damage) {
    velocity_ = {direction.x * speed, direction.y * speed, direction.z * speed};
    damage_ = damage;
    lifeTimer_ = 0.0f;
}

void EnemyBulletComponent::ResetForPool() {
    velocity_ = {0.0f, 0.0f, 0.0f};
    damage_ = 10;
    lifeTimer_ = 0.0f;
}

void EnemyBulletComponent::Deactivate() {
    if (manager_) {
        manager_->ReturnBullet(this);
    } else if (gameObject_) {
        gameObject_->Destroy();
    }
}

void EnemyBulletComponent::Update() {
    if (!gameObject_) {
        return;
    }

    auto engine = GetEngine();
    if (!engine) {
        return;
    }

    float dt = engine->GetGameDeltaTime();
    if (dt <= 0.0f) {
        return;
    }

    lifeTimer_ += dt;
    if (lifeTimer_ >= maxLifeTime_) {
        Deactivate();
        return;
    }

    if (auto transform = GetTransform()) {
        Irufemi::Vector3 pos = transform->GetWorldPosition();
        pos.x += velocity_.x * dt;
        pos.y += velocity_.y * dt;
        pos.z += velocity_.z * dt;
        transform->SetWorldPosition(pos);
    }
}

void EnemyBulletComponent::OnCollisionEnter(GameObject* other) {
    if (!other || !gameObject_) {
        return;
    }

    // プレイヤーへの命中判定
    if (auto health = other->GetComponent<PlayerHealthComponent>()) {
        if (!health->IsInvincible()) {
            health->TakeDamage(damage_);

            // 被弾エフェクトの再生
            if (auto transform = GetTransform()) {
                if (auto scene = gameObject_->GetScene()) {
                    if (auto effectGo = scene->FindGameObject("EffectManager")) {
                        if (auto effectMgr = effectGo->GetComponent<EffectManagerComponent>()) {
                            effectMgr->PlayEffect("debris_dust_effect", transform->GetWorldPosition());
                        }
                    }
                }
            }

            // 弾自身を返却（または破棄）
            Deactivate();
        }
    }
}
