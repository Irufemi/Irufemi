#include "Environment/DebrisComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"
#include "RailMechanics/RailShooterEnemyComponent.h"
#include "Combat/Boss/BossComponent.h"
#include "Environment/DebrisManagerComponent.h"
#include "Effects/EffectManagerComponent.h"
#include "Core/Math/Random/Random.h"
#include "Core/Math/MathFunction.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Renderer/Camera/CameraManager.h"
#include "Renderer/System/VoxelParticle/VoxelParticleManager.h"
#include "Environment/DestructibleEnvironmentComponent.h"
#include "Framework/Component/Camera/CameraShakeComponent.h"
#include "Framework/Scene/BaseScene.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Renderer/Object/3D/Primitive/Primitive3DObject.h"
#include <cmath>
#include <windows.h>
#include <iostream>
#include "Core/Utility/Log.h"
#include "Physics/CollisionManager.h"

static ColliderComponent* GetColliderFromObj(GameObject* obj) {
    if (!obj)
        return nullptr;
    for (auto& comp : obj->GetComponents()) {
        if (auto col = dynamic_cast<ColliderComponent*>(comp.get())) {
            return col;
        }
    }
    return nullptr;
}

float DebrisComponent::GetPullSpeed() const {
    return manager_ ? manager_->GetDebrisPullSpeed() : 10.0f;
}
float DebrisComponent::GetThrowSpeed() const {
    return manager_ ? manager_->GetDebrisThrowSpeed() : 50.0f;
}
float DebrisComponent::GetOrbitSpeed() const {
    return manager_ ? manager_->GetDebrisOrbitSpeed() : 2.0f;
}
float DebrisComponent::GetBossDamage() const {
    return manager_ ? manager_->GetDebrisDamage() : 10.0f;
}
float DebrisComponent::GetEnemyDamage() const {
    return manager_ ? manager_->GetDebrisEnemyDamage() : 100.0f;
}
float DebrisComponent::GetCameraShakeIntensity() const {
    return manager_ ? manager_->GetCameraShakeIntensity() : 0.5f;
}
int DebrisComponent::GetCameraShakeDurationFrames() const {
    return manager_ ? manager_->GetCameraShakeDurationFrames() : 10;
}
Irufemi::Vector4 DebrisComponent::GetPlayerAuraColor() const {
    return manager_ ? manager_->GetPlayerAuraColor() : Irufemi::Vector4{0.0f, 0.8f, 1.0f, 0.4f};
}
Irufemi::Vector4 DebrisComponent::GetBossAuraColor() const {
    return manager_ ? manager_->GetBossAuraColor() : Irufemi::Vector4{0.8f, 0.0f, 0.6f, 0.4f};
}
float DebrisComponent::GetCatchDistanceSq() const {
    return manager_ ? manager_->GetCatchDistanceSq() : 2.0f;
}
float DebrisComponent::GetBossShieldRadius() const {
    return manager_ ? manager_->GetBossShieldRadius() : 8.0f;
}
float DebrisComponent::GetPullYOffset() const {
    return manager_ ? manager_->GetDebrisPullYOffset() : 2.0f;
}

void DebrisComponent::OnRegisterProperties() {
    RegisterProperty("Hit Effect Key", &hitEffectKey_);
    RegisterProperty("Explosion Model Path", &explosionModelPath_);
}

void DebrisComponent::Initialize() {
    // 必要なコンポーネントのキャッシュや初期化のみ行う
}

void DebrisComponent::OnEnable() {
    state_ = DebrisState::Idle;
    targetObject_.reset();
    idleTimeY_ = static_cast<float>(rand() % 100); // ランダムな位相で開始

    if (auto transform = GetTransform()) {
        baseIdleY_ = transform->GetPosition().y;
    }
}

void DebrisComponent::OnDisable() {
    if (manager_) {
        manager_->UnregisterDebris(this, state_);
    }
}

void DebrisComponent::OnCollisionEnter(GameObject* otherObj) {
    if (state_ != DebrisState::Thrown)
        return;
    if (!otherObj)
        return;

    bool hit = false;
    if (auto enemyComp = otherObj->GetComponent<RailShooterEnemyComponent>()) {
        enemyComp->TakeDamage(static_cast<int>(GetEnemyDamage()));
        hit = true;
    } else if (auto bossComp = otherObj->GetComponent<BossComponent>()) {
        bossComp->TakeDamage(GetBossDamage());
        hit = true;
    } else if (auto debrisComp = otherObj->GetComponent<DebrisComponent>()) {
        if (debrisComp->GetState() == DebrisState::BossOrbiting) {
            // Bossからシールドを解除する
            if (auto bossTarget = debrisComp->GetTarget().lock()) {
                if (auto bossTargetComp = bossTarget->GetComponent<BossComponent>()) {
                    bossTargetComp->RemoveShield(otherObj->shared_from_this());
                }
            }

            // シールドを消滅させる
            if (debrisComp->manager_) {
                debrisComp->manager_->MarkForRelease(otherObj->shared_from_this());
                if (debrisComp->virtualId_ >= 0) {
                    debrisComp->manager_->MarkForDestroy(debrisComp->virtualId_, debrisComp->variationIndex_);
                }
            } else {
                otherObj->SetIsActive(false);
            }
            hit = true;
        }
    } else if (auto destructible = otherObj->GetComponent<DestructibleEnvironmentComponent>()) {
        destructible->TakeDamage(1);
        hit = true;
    } else if (auto collider = GetColliderFromObj(otherObj)) {
        auto cm = BaseModel::GetIrufemiEngine()->GetCollisionManager();
        // 建造物（Environmentレイヤー）との衝突検知
        // 衝突した場合は破砕エフェクトを再生し、プールへ返却（回収）する
        if (cm) {
            uint32_t envMask = cm->GetLayerMask("Environment");
            if ((collider->layer_ & envMask) != 0) {
                hit = true;
            }
        }
    }

    if (hit) {
        if (auto t = GetTransform()) {
            Irufemi::Vector3 hitPos = t->GetWorldPosition();

            EffectManagerComponent* effectManager = nullptr;
            if (auto go = gameObject_->GetScene()->FindGameObject("EffectManager")) {
                effectManager = go->GetComponent<EffectManagerComponent>();
            }
            if (effectManager) {
                effectManager->PlayEffect(hitEffectKey_, hitPos);
            }

            if (auto voxelManager = BaseModel::GetIrufemiEngine()->GetVoxelParticleManager()) {
                VoxelEmitter p{};
                p.particleType = 5; // DebrisExplosive
                p.lifeTime = 1.0f;
                p.gravity = 5.0f;
                p.dispersion = 12.0f;
                p.scale = {0.5f, 0.5f, 0.5f};

                Irufemi::Vector4 aura =
                    (state_ == DebrisState::BossOrbiting) ? GetBossAuraColor() : GetPlayerAuraColor();
                Irufemi::Vector4 rockColor = {1.5f, 1.2f, 1.0f, 1.0f};
                p.startColor = {rockColor.x + aura.x * 2.0f, rockColor.y + aura.y * 2.0f, rockColor.z + aura.z * 2.0f,
                                1.0f};
                p.endColor = {0.2f, 0.2f, 0.2f, 1.0f};
                p.dissolveEdgeColor = aura;

                voxelManager->PlayExplosion(explosionModelPath_, hitPos, {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, p, {2, 2, 2});
            }
        }
        if (manager_) {
            manager_->MarkForRelease(gameObject_->shared_from_this());
            if (virtualId_ >= 0) {
                manager_->MarkForDestroy(virtualId_, variationIndex_);
            }
        } else {
            gameObject_->SetIsActive(false);
        }
    }
}

void DebrisComponent::SetState(DebrisState newState) {
    if (state_ == newState)
        return;
    if (manager_) {
        manager_->UnregisterDebris(this, state_);
    }
    state_ = newState;
    if (manager_) {
        manager_->RegisterDebris(this, state_);
    }

    // オーラ用子オブジェクトの表示切り替えと色の変更
    if (gameObject_) {
        for (auto& child : gameObject_->GetChildren()) {
            if (child && child->GetName() == "DebrisAura") {
                bool isActive = false;
                Irufemi::Vector4 auraColor = {1.0f, 1.0f, 1.0f, 0.7f};

                switch (state_) {
                case DebrisState::Pulled:
                case DebrisState::Orbiting:
                case DebrisState::Thrown:
                    isActive = true;
                    auraColor = GetPlayerAuraColor();
                    break;
                case DebrisState::BossOrbiting:
                    isActive = true;
                    auraColor = GetBossAuraColor();
                    break;
                default:
                    break;
                }

                child->SetIsActive(isActive);

                if (isActive) {
                    if (auto auraModel = child->GetComponent<PrimitiveRendererComponent>()) {
                        if (auto primitive = static_cast<Primitive3DObject*>(auraModel->GetRenderable())) {
                            primitive->SetColor(auraColor);
                        }
                    }
                }
            }
        }
    }

    if (auto collider = GetColliderFromObj(gameObject_)) {
        auto* cm = BaseModel::GetIrufemiEngine()->GetCollisionManager();
        if (cm) {
            uint32_t neutralLayer = cm->GetLayerMask("Debris_Neutral");
            uint32_t playerLayer = cm->GetLayerMask("Debris_Player");
            uint32_t enemyLayer = cm->GetLayerMask("Debris_Enemy");

            uint32_t maskEnemy = cm->GetLayerMask("Enemy");
            uint32_t maskPlayer = cm->GetLayerMask("Player");
            uint32_t maskEnvironment = cm->GetLayerMask("Environment");

            switch (state_) {
            case DebrisState::Idle:
            case DebrisState::Pulled:
            case DebrisState::Orbiting:
                // Safe state: Doesn't hit anyone
                collider->layer_ = neutralLayer;
                collider->mask_ = 0; // Collides with nothing in this prototype
                break;
            case DebrisState::Thrown:
                // Thrown by player: Hits enemies, environment, and Boss's debris
                collider->layer_ = playerLayer;
                collider->mask_ = maskEnemy | maskEnvironment | enemyLayer;
                break;
            case DebrisState::BossOrbiting:
                // Used by Boss: Hits player and Player's thrown debris
                collider->layer_ = enemyLayer;
                collider->mask_ = maskPlayer | playerLayer;
                break;
            }
        }
    }

    if (state_ == DebrisState::BossOrbiting) {
        bossOrbitAngleX_ = Irufemi::Random::GeneratorFloat(0.0f, Irufemi::Math::PI * 2.0f);
        bossOrbitAngleY_ = Irufemi::Random::GeneratorFloat(0.0f, Irufemi::Math::PI * 2.0f);
        bossOrbitAngleZ_ = Irufemi::Random::GeneratorFloat(0.0f, Irufemi::Math::PI * 2.0f);
        bossOrbitSpeedX_ = Irufemi::Random::GeneratorFloat(-1.2f, 1.2f);
        bossOrbitSpeedY_ = Irufemi::Random::GeneratorFloat(-3.0f, 3.0f);
        bossOrbitSpeedZ_ = Irufemi::Random::GeneratorFloat(-1.2f, 1.2f);
        bossOrbitRadiusOffset_ = Irufemi::Random::GeneratorFloat(-1.0f, 1.0f);
    }

    if (state_ == DebrisState::Thrown && gameObject_) {
        if (auto transform = GetTransform()) {
            throwOrigin_ = transform->GetWorldPosition();
        }
    }
}

std::shared_ptr<Component> DebrisComponent::Clone() {
    auto clone = std::make_shared<DebrisComponent>();
    clone->CopyPropertiesFrom(this);
    clone->state_ = this->state_;
    clone->virtualId_ = this->virtualId_;
    clone->variationIndex_ = this->variationIndex_;
    clone->manager_ = this->manager_;
    clone->targetObject_ = this->targetObject_;
    // No need to copy internal state variables like idleTimeY_, baseIdleY_ deeply, but doing default member copy is
    // fine since it's a new instance.
    return clone;
}
