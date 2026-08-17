#include "DebrisComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"
#include "RailShooterEnemyComponent.h"
#include "Boss/BossComponent.h"
#include "DebrisManagerComponent.h"
#include "EffectManagerComponent.h"
#include "Engine/Core/Math/Random/Random.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "DestructibleEnvironmentComponent.h"
#include "Framework/Component/Camera/CameraShakeComponent.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Renderer/Object/3D/Primitive/Primitive3DObject.h"
#include <cmath>
#include <windows.h>
#include <iostream>
#include "Engine/Core/Utility/Log.h"
#include "Engine/Manager/CollisionManager.h"

static ColliderComponent* GetColliderFromObj(GameObject* obj) {
    if (!obj) return nullptr;
    for (auto& comp : obj->GetComponents()) {
        if (auto col = dynamic_cast<ColliderComponent*>(comp.get())) {
            return col;
        }
    }
    return nullptr;
}


float DebrisComponent::GetPullSpeed() const { return manager_ ? manager_->GetDebrisPullSpeed() : 10.0f; }
float DebrisComponent::GetThrowSpeed() const { return manager_ ? manager_->GetDebrisThrowSpeed() : 50.0f; }
float DebrisComponent::GetOrbitSpeed() const { return manager_ ? manager_->GetDebrisOrbitSpeed() : 2.0f; }
float DebrisComponent::GetBossDamage() const { return manager_ ? manager_->GetDebrisDamage() : 10.0f; }
float DebrisComponent::GetEnemyDamage() const { return manager_ ? manager_->GetDebrisEnemyDamage() : 100.0f; }
float DebrisComponent::GetCameraShakeIntensity() const { return manager_ ? manager_->GetCameraShakeIntensity() : 0.5f; }
int DebrisComponent::GetCameraShakeDurationFrames() const { return manager_ ? manager_->GetCameraShakeDurationFrames() : 10; }
Irufemi::Vector4 DebrisComponent::GetPlayerAuraColor() const { return manager_ ? manager_->GetPlayerAuraColor() : Irufemi::Vector4{0.0f, 0.8f, 1.0f, 0.4f}; }
Irufemi::Vector4 DebrisComponent::GetBossAuraColor() const { return manager_ ? manager_->GetBossAuraColor() : Irufemi::Vector4{0.8f, 0.0f, 0.6f, 0.4f}; }
float DebrisComponent::GetCatchDistanceSq() const { return manager_ ? manager_->GetCatchDistanceSq() : 2.0f; }
float DebrisComponent::GetBossShieldRadius() const { return manager_ ? manager_->GetBossShieldRadius() : 8.0f; }
float DebrisComponent::GetPullYOffset() const { return manager_ ? manager_->GetDebrisPullYOffset() : 2.0f; }

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

void DebrisComponent::OnCollisionEnter(GameObject* otherObj) {
    if (state_ != DebrisState::Thrown) return;
    if (!otherObj) return;

    Log::OutPutLog(std::cout, "Debris (Thrown) OnCollisionEnter with: " + otherObj->GetName() + "\n");

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
            
            // シールド側を消滅させる
            if (debrisComp->manager_) {
                debrisComp->manager_->ReleaseDebris(otherObj->shared_from_this());
                if (debrisComp->virtualId_ >= 0) {
                    debrisComp->manager_->NotifyDestroyed(debrisComp->virtualId_, debrisComp->variationIndex_);
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
            Log::OutPutLog(std::cout, "Checking Environment collision. Collider Layer: " + std::to_string(collider->layer_) + ", EnvMask: " + std::to_string(envMask) + "\n");
            if ((collider->layer_ & envMask) != 0) {
                Log::OutPutLog(std::cout, "Environment Hit Detected!\n");
                hit = true;
            } else {
                Log::OutPutLog(std::cout, "Not Environment Layer. Collision ignored.\n");
            }
        }
    }

    if (hit) {

        if (auto t = GetTransform()) {
            if (auto effectManager = EffectManagerComponent::GetInstance()) {
                effectManager->PlayEffect("Hit", t->GetWorldPosition());
            }
        }
        if (manager_) {
            manager_->ReleaseDebris(gameObject_->shared_from_this());
            if (virtualId_ >= 0) {
                manager_->NotifyDestroyed(virtualId_, variationIndex_);
            }
        } else {
            gameObject_->SetIsActive(false); 
        }
    }
}

void DebrisComponent::SetState(DebrisState newState) {
    state_ = newState;

    // オーラ用子オブジェクトの表示切り替えと色の変更
    if (gameObject_) {
        for (auto& child : gameObject_->GetChildren()) {
            if (child && child->GetName() == "DebrisAura") {
                bool isActive = false;
                Irufemi::Vector4 auraColor = { 1.0f, 1.0f, 1.0f, 0.7f };

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

void DebrisComponent::Update() {
    if (!gameObject_) return;
    auto transform = GetTransform();
    if (!transform) return;

    float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (deltaTime <= 0.0f) return;

    switch (state_) {
        case DebrisState::Idle: {
            // アニメーション計算は VirtualEntityManager(DebrisManager) 側で行うため、ここでは何もしない
            break;
        }
        case DebrisState::Pulled: {
            if (auto target = targetObject_.lock()) {
                auto targetTransform = target->GetComponent<TransformComponent>();
                if (targetTransform) {
                    Irufemi::Vector3 targetPos = targetTransform->GetWorldPosition();
                    // ターゲット(プレイヤー)に向かってLerpで移動
                    Irufemi::Vector3 diff = {
                        targetPos.x - transform->GetWorldPosition().x,
                        targetPos.y + GetPullYOffset() - transform->GetWorldPosition().y, // 少し上に引き寄せる
                        targetPos.z - transform->GetWorldPosition().z
                    };
                    Irufemi::Vector3 pos = transform->GetWorldPosition();
                    pos.x += diff.x * GetPullSpeed() * deltaTime;
                    pos.y += diff.y * GetPullSpeed() * deltaTime;
                    pos.z += diff.z * GetPullSpeed() * deltaTime;
                    transform->SetPosition(pos);

                    // 一定距離に近づいたらOrbitingへ自動遷移
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    if (distSq < GetCatchDistanceSq()) {
                        SetState(DebrisState::Orbiting);
                    }
                }
            }
            break;
        }
        case DebrisState::Orbiting: {
            if (auto target = targetObject_.lock()) {
                auto targetTransform = target->GetComponent<TransformComponent>();
                if (targetTransform) {
                    orbitAngle_ += GetOrbitSpeed() * deltaTime;
                    
                    // プレイヤーの周囲を回転するローカル座標を計算
                    Irufemi::Vector3 offset = {
                        std::cos(orbitAngle_) * orbitRadius_,
                        std::sin(orbitAngle_ * 2.0f) * 0.5f + 1.0f, // 8の字にフワフワ
                        std::sin(orbitAngle_) * orbitRadius_
                    };
                    
                    Irufemi::Vector3 pos = transform->GetWorldPosition();
                    pos.x = targetTransform->GetWorldPosition().x + offset.x;
                    pos.y = targetTransform->GetWorldPosition().y + offset.y;
                    pos.z = targetTransform->GetWorldPosition().z + offset.z;
                    transform->SetPosition(pos);
                }
            }
            break;
        }
        case DebrisState::BossOrbiting: {
            if (auto target = targetObject_.lock()) {
                auto targetTransform = target->GetComponent<TransformComponent>();
                if (targetTransform) {
                    float shieldRotationSpeed = 1.0f; // ボス側のパラメータを取得してもよいがとりあえず固定値
                    bossOrbitAngleX_ += bossOrbitSpeedX_ * shieldRotationSpeed * deltaTime;
                    bossOrbitAngleY_ += bossOrbitSpeedY_ * shieldRotationSpeed * deltaTime;
                    bossOrbitAngleZ_ += bossOrbitSpeedZ_ * shieldRotationSpeed * deltaTime;

                    Irufemi::Matrix4x4 rotMatrix = Irufemi::Math::MakeRotateXYZMatrix(Irufemi::Vector3{bossOrbitAngleX_, bossOrbitAngleY_, bossOrbitAngleZ_});
                    float currentRadius = GetBossShieldRadius() + bossOrbitRadiusOffset_;
                    Irufemi::Vector3 baseOffset = { 0, 0, currentRadius };
                    Irufemi::Vector3 localPos = Irufemi::Math::TransformNormal(baseOffset, rotMatrix);

                    Irufemi::Vector3 pos = transform->GetWorldPosition();
                    pos.x = targetTransform->GetWorldPosition().x + localPos.x;
                    pos.y = targetTransform->GetWorldPosition().y + localPos.y;
                    pos.z = targetTransform->GetWorldPosition().z + localPos.z;
                    transform->SetPosition(pos);

                    // 自身の回転も反映（2倍の速度で自転）
                    transform->SetRotation({ bossOrbitAngleX_ * 2.0f, bossOrbitAngleY_ * 2.0f, bossOrbitAngleZ_ * 2.0f });
                }
            }
            break;
        }
        case DebrisState::Thrown: {
            if (auto target = targetObject_.lock(); target && target->GetIsActive()) {
                auto targetTransform = target->GetComponent<TransformComponent>();
                if (targetTransform) {
                    // 敵に向かって高速ホーミング移動
                    Irufemi::Vector3 diff = {
                        targetTransform->GetWorldPosition().x - transform->GetWorldPosition().x,
                        targetTransform->GetWorldPosition().y - transform->GetWorldPosition().y,
                        targetTransform->GetWorldPosition().z - transform->GetWorldPosition().z
                    };
                    float len = std::sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
                    float moveDist = GetThrowSpeed() * deltaTime;
                    if (len > 0.001f) {
                        if (len <= moveDist) {
                            throwDirection_ = { diff.x / len, diff.y / len, diff.z / len };
                            Irufemi::Vector3 pos = transform->GetWorldPosition();
                            pos.x += diff.x;
                            pos.y += diff.y;
                            pos.z += diff.z;
                            transform->SetPosition(pos);
                            return;
                        } else {
                            throwDirection_ = { diff.x / len, diff.y / len, diff.z / len };
                        }
                    }
                }
            }
            
            // ターゲットがない（または既に死んだ）場合でも、計算された(または初期設定された)方向に飛び続ける
            Irufemi::Vector3 pos = transform->GetWorldPosition();
            pos.x += throwDirection_.x * GetThrowSpeed() * deltaTime;
            pos.y += throwDirection_.y * GetThrowSpeed() * deltaTime;
            pos.z += throwDirection_.z * GetThrowSpeed() * deltaTime;
            transform->SetPosition(pos);
            
            // 限界距離でのデスポーン（オブジェクトプール返却）
            if (manager_) {
                float dx = pos.x - throwOrigin_.x;
                float dy = pos.y - throwOrigin_.y;
                float dz = pos.z - throwOrigin_.z;
                float distSq = dx*dx + dy*dy + dz*dz;
                
                if (distSq > manager_->GetMaxThrowDistanceSq()) {
                    if (auto effectManager = EffectManagerComponent::GetInstance()) {
                        effectManager->PlayEffect("Hit", pos);
                    }
                    manager_->MarkForRelease(gameObject_->shared_from_this());
                }
            } else {
                // Managerがない場合のフォールバック（デバッグ用など）
                float dx = pos.x - throwOrigin_.x;
                float dy = pos.y - throwOrigin_.y;
                float dz = pos.z - throwOrigin_.z;
                float distSq = dx*dx + dy*dy + dz*dz;
                if (distSq > 1500.0f * 1500.0f) {
                    gameObject_->SetIsActive(false);
                }
            }
            break;
        }
    }
}
