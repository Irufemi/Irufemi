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
#include <cmath>

void DebrisComponent::OnRegisterProperties() {
    RegisterProperty("Pull Speed", &pullSpeed_);
    RegisterProperty("Throw Speed", &throwSpeed_);
    RegisterProperty("Orbit Speed", &orbitSpeed_);
}

void DebrisComponent::Initialize() {
    state_ = DebrisState::Idle;
    targetObject_.reset();
    idleTimeY_ = static_cast<float>(rand() % 100); // ランダムな位相で開始
    
    if (auto transform = gameObject_->GetComponent<TransformComponent>()) {
        baseIdleY_ = transform->GetPosition().y;
    }

    if (auto collider = gameObject_->GetComponent<SphereColliderComponent>()) {
        collider->onCollisionEnter_ = [this](ColliderComponent* other) {
            if (state_ != DebrisState::Thrown) return;
            
            auto otherObj = other->GetGameObject();
            if (!otherObj) return;

            bool hit = false;
            if (auto enemyComp = otherObj->GetComponent<RailShooterEnemyComponent>()) {
                enemyComp->TakeDamage(100);
                hit = true;
            } else if (auto bossComp = otherObj->GetComponent<BossComponent>()) {
                bossComp->TakeDamage(10.0f);
                hit = true;
            }

            if (hit) {
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
        };
    }
}

void DebrisComponent::SetState(DebrisState newState) {
    state_ = newState;
    if (state_ == DebrisState::BossOrbiting) {
        bossOrbitAngleX_ = Random::GeneratorFloat(0.0f, Math::PI * 2.0f);
        bossOrbitAngleY_ = Random::GeneratorFloat(0.0f, Math::PI * 2.0f);
        bossOrbitAngleZ_ = Random::GeneratorFloat(0.0f, Math::PI * 2.0f);
        bossOrbitSpeedX_ = Random::GeneratorFloat(-0.02f, 0.02f);
        bossOrbitSpeedY_ = Random::GeneratorFloat(-0.05f, 0.05f);
        bossOrbitSpeedZ_ = Random::GeneratorFloat(-0.02f, 0.02f);
        bossOrbitRadiusOffset_ = Random::GeneratorFloat(-1.0f, 1.0f);
    }
}

void DebrisComponent::Update() {
    if (!gameObject_) return;
    auto transform = gameObject_->GetComponent<TransformComponent>();
    if (!transform) return;

    float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (deltaTime <= 0.0f) deltaTime = 1.0f / 60.0f;

    switch (state_) {
        case DebrisState::Idle: {
            // アニメーション計算は VirtualEntityManager(DebrisManager) 側で行うため、ここでは何もしない
            break;
        }
        case DebrisState::Pulled: {
            if (auto target = targetObject_.lock()) {
                auto targetTransform = target->GetComponent<TransformComponent>();
                if (targetTransform) {
                    // ターゲット(プレイヤー)に向かってLerpで移動
                    Vector3 diff = {
                        targetTransform->GetPosition().x - transform->GetPosition().x,
                        targetTransform->GetPosition().y + 2.0f - transform->GetPosition().y, // 少し上に引き寄せる
                        targetTransform->GetPosition().z - transform->GetPosition().z
                    };
                    Vector3 pos = transform->GetPosition();
                    pos.x += diff.x * pullSpeed_ * deltaTime;
                    pos.y += diff.y * pullSpeed_ * deltaTime;
                    pos.z += diff.z * pullSpeed_ * deltaTime;
                    transform->SetPosition(pos);

                    // 一定距離に近づいたらOrbitingへ自動遷移
                    float distSq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
                    if (distSq < 2.0f) {
                        state_ = DebrisState::Orbiting;
                    }
                }
            }
            break;
        }
        case DebrisState::Orbiting: {
            if (auto target = targetObject_.lock()) {
                auto targetTransform = target->GetComponent<TransformComponent>();
                if (targetTransform) {
                    orbitAngle_ += orbitSpeed_ * deltaTime;
                    
                    // プレイヤーの周囲を回転するローカル座標を計算
                    Vector3 offset = {
                        std::cos(orbitAngle_) * orbitRadius_,
                        std::sin(orbitAngle_ * 2.0f) * 0.5f + 1.0f, // 8の字にフワフワ
                        std::sin(orbitAngle_) * orbitRadius_
                    };
                    
                    Vector3 pos = transform->GetPosition();
                    pos.x = targetTransform->GetPosition().x + offset.x;
                    pos.y = targetTransform->GetPosition().y + offset.y;
                    pos.z = targetTransform->GetPosition().z + offset.z;
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
                    bossOrbitAngleX_ += bossOrbitSpeedX_ * shieldRotationSpeed;
                    bossOrbitAngleY_ += bossOrbitSpeedY_ * shieldRotationSpeed;
                    bossOrbitAngleZ_ += bossOrbitSpeedZ_ * shieldRotationSpeed;

                    Matrix4x4 rotMatrix = Math::MakeRotateXYZMatrix(Vector3{bossOrbitAngleX_, bossOrbitAngleY_, bossOrbitAngleZ_});
                    float bossShieldRadius = 8.0f; // 基本半径
                    Vector3 baseOffset = { 0, 0, bossShieldRadius + bossOrbitRadiusOffset_ };
                    Vector3 localPos = Math::TransformNormal(baseOffset, rotMatrix);

                    Vector3 pos = transform->GetPosition();
                    pos.x = targetTransform->GetPosition().x + localPos.x;
                    pos.y = targetTransform->GetPosition().y + localPos.y;
                    pos.z = targetTransform->GetPosition().z + localPos.z;
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
                        targetTransform->GetPosition().x - transform->GetPosition().x,
                        targetTransform->GetPosition().y - transform->GetPosition().y,
                        targetTransform->GetPosition().z - transform->GetPosition().z
                    };
                    // 正規化して一定速度で飛ばす
                    float len = std::sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
                    if (len > 0.001f) {
                        throwDirection_ = { diff.x / len, diff.y / len, diff.z / len };
                    }
                }
            }
            
            // ターゲットがない（または既に死んだ）場合でも、計算された(または初期設定された)方向に飛び続ける
            Vector3 pos = transform->GetPosition();
            pos.x += throwDirection_.x * throwSpeed_ * deltaTime;
            pos.y += throwDirection_.y * throwSpeed_ * deltaTime;
            pos.z += throwDirection_.z * throwSpeed_ * deltaTime;
            transform->SetPosition(pos);
            
            // TODO: 一定距離/時間で消滅させる等の処理が必要
            break;
        }
    }
}
