#include "DebrisComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"
#include "RailShooterEnemyComponent.h"
#include "BossComponent.h"
#include "DebrisManagerComponent.h"
#include "EffectManagerComponent.h"
#include "Engine/Core/Math/Random/Random.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Renderer/Object/3D/Primitive/Primitive3DObject.h"
#include <cmath>



float DebrisComponent::GetPullSpeed() const { return manager_ ? manager_->GetDebrisPullSpeed() : 10.0f; }
float DebrisComponent::GetThrowSpeed() const { return manager_ ? manager_->GetDebrisThrowSpeed() : 50.0f; }
float DebrisComponent::GetOrbitSpeed() const { return manager_ ? manager_->GetDebrisOrbitSpeed() : 2.0f; }
float DebrisComponent::GetBossDamage() const { return manager_ ? manager_->GetDebrisDamage() : 10.0f; }
float DebrisComponent::GetEnemyDamage() const { return manager_ ? manager_->GetDebrisEnemyDamage() : 100.0f; }
float DebrisComponent::GetCameraShakeIntensity() const { return manager_ ? manager_->GetCameraShakeIntensity() : 0.5f; }
int DebrisComponent::GetCameraShakeDurationFrames() const { return manager_ ? manager_->GetCameraShakeDurationFrames() : 10; }
Vector4 DebrisComponent::GetPlayerAuraColor() const { return manager_ ? manager_->GetPlayerAuraColor() : Vector4{0.0f, 0.8f, 1.0f, 0.4f}; }
Vector4 DebrisComponent::GetBossAuraColor() const { return manager_ ? manager_->GetBossAuraColor() : Vector4{0.8f, 0.0f, 0.6f, 0.4f}; }
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
    
    if (auto transform = gameObject_->GetComponent<TransformComponent>()) {
        baseIdleY_ = transform->GetPosition().y;
    }
}

void DebrisComponent::OnCollisionEnter(GameObject* otherObj) {
    if (state_ != DebrisState::Thrown) return;
    if (!otherObj) return;

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
                    debrisComp->manager_->NotifyDestroyed(debrisComp->virtualId_);
                }
            } else {
                otherObj->SetIsActive(false);
            }
            hit = true;
        }
    }

    if (hit) {
        // 軽いカメラシェイクを追加
        if (auto camera = BaseModel::GetIrufemiEngine()->GetCameraManager()->GetActiveCamera()) {
            camera->Shake(GetCameraShakeIntensity(), GetCameraShakeDurationFrames());
        }
        if (auto t = gameObject_->GetComponent<TransformComponent>()) {
            if (auto effectManager = EffectManagerComponent::GetInstance()) {
                effectManager->PlayEffect("Hit", t->GetWorldPosition());
            }
        }
        if (manager_) {
            manager_->ReleaseDebris(gameObject_->shared_from_this());
            if (virtualId_ >= 0) {
                manager_->NotifyDestroyed(virtualId_);
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
                Vector4 auraColor = { 1.0f, 1.0f, 1.0f, 0.7f };

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

    if (state_ == DebrisState::BossOrbiting) {
        bossOrbitAngleX_ = Random::GeneratorFloat(0.0f, Math::PI * 2.0f);
        bossOrbitAngleY_ = Random::GeneratorFloat(0.0f, Math::PI * 2.0f);
        bossOrbitAngleZ_ = Random::GeneratorFloat(0.0f, Math::PI * 2.0f);
        bossOrbitSpeedX_ = Random::GeneratorFloat(-1.2f, 1.2f);
        bossOrbitSpeedY_ = Random::GeneratorFloat(-3.0f, 3.0f);
        bossOrbitSpeedZ_ = Random::GeneratorFloat(-1.2f, 1.2f);
        bossOrbitRadiusOffset_ = Random::GeneratorFloat(-1.0f, 1.0f);
    }
}

void DebrisComponent::Update() {
    if (!gameObject_) return;
    auto transform = gameObject_->GetComponent<TransformComponent>();
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
                    Vector3 targetPos = targetTransform->GetWorldPosition();
                    // ターゲット(プレイヤー)に向かってLerpで移動
                    Vector3 diff = {
                        targetPos.x - transform->GetWorldPosition().x,
                        targetPos.y + GetPullYOffset() - transform->GetWorldPosition().y, // 少し上に引き寄せる
                        targetPos.z - transform->GetWorldPosition().z
                    };
                    Vector3 pos = transform->GetWorldPosition();
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
                    Vector3 offset = {
                        std::cos(orbitAngle_) * orbitRadius_,
                        std::sin(orbitAngle_ * 2.0f) * 0.5f + 1.0f, // 8の字にフワフワ
                        std::sin(orbitAngle_) * orbitRadius_
                    };
                    
                    Vector3 pos = transform->GetWorldPosition();
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

                    Matrix4x4 rotMatrix = Math::MakeRotateXYZMatrix(Vector3{bossOrbitAngleX_, bossOrbitAngleY_, bossOrbitAngleZ_});
                    float currentRadius = GetBossShieldRadius() + bossOrbitRadiusOffset_;
                    Vector3 baseOffset = { 0, 0, currentRadius };
                    Vector3 localPos = Math::TransformNormal(baseOffset, rotMatrix);

                    Vector3 pos = transform->GetWorldPosition();
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
                    Vector3 diff = {
                        targetTransform->GetWorldPosition().x - transform->GetWorldPosition().x,
                        targetTransform->GetWorldPosition().y - transform->GetWorldPosition().y,
                        targetTransform->GetWorldPosition().z - transform->GetWorldPosition().z
                    };
                    // 正規化して一定速度で飛ばす
                    float len = std::sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
                    if (len > 0.001f) {
                        throwDirection_ = { diff.x / len, diff.y / len, diff.z / len };
                    }
                }
            }
            
            // ターゲットがない（または既に死んだ）場合でも、計算された(または初期設定された)方向に飛び続ける
            Vector3 pos = transform->GetWorldPosition();
            pos.x += throwDirection_.x * GetThrowSpeed() * deltaTime;
            pos.y += throwDirection_.y * GetThrowSpeed() * deltaTime;
            pos.z += throwDirection_.z * GetThrowSpeed() * deltaTime;
            transform->SetPosition(pos);
            
            // TODO: 一定距離/時間で消滅させる等の処理が必要
            break;
        }
    }
}
