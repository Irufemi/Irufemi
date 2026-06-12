#include "DebrisComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Renderer/System/Core/BaseModel.h"
#include "RailShooterEnemyComponent.h"
#include "BossComponent.h"
#include "DebrisManagerComponent.h"
#include "Engine/Core/Math/Random/Random.h"
#include "Engine/Core/Math/MathFunction.h"
#include <cmath>

void DebrisComponent::OnRegisterProperties() {
    RegisterProperty("Pull Speed", &pullSpeed_);
    RegisterProperty("Throw Speed", &throwSpeed_);
    RegisterProperty("Orbit Speed", &orbitSpeed_);
}

void DebrisComponent::Initialize() {
    state_ = DebrisState::Idle;
    targetObject_ = nullptr;
    idleTimeY_ = static_cast<float>(rand() % 100); // ランダムな位相で開始
    
    if (auto transform = gameObject_->GetComponent<TransformComponent>()) {
        baseIdleY_ = transform->position_.y;
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
            // フワフワと上下に漂う疑似アニメーション
            idleTimeY_ += deltaTime * 2.0f;
            transform->position_.y = baseIdleY_ + std::sin(idleTimeY_) * 0.5f;
            break;
        }
        case DebrisState::Pulled: {
            if (targetObject_) {
                auto targetTransform = targetObject_->GetComponent<TransformComponent>();
                if (targetTransform) {
                    // ターゲット(プレイヤー)に向かってLerpで移動
                    Vector3 diff = {
                        targetTransform->position_.x - transform->position_.x,
                        targetTransform->position_.y + 2.0f - transform->position_.y, // 少し上に引き寄せる
                        targetTransform->position_.z - transform->position_.z
                    };
                    transform->position_.x += diff.x * pullSpeed_ * deltaTime;
                    transform->position_.y += diff.y * pullSpeed_ * deltaTime;
                    transform->position_.z += diff.z * pullSpeed_ * deltaTime;

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
            if (targetObject_) {
                auto targetTransform = targetObject_->GetComponent<TransformComponent>();
                if (targetTransform) {
                    orbitAngle_ += orbitSpeed_ * deltaTime;
                    
                    // プレイヤーの周囲を回転するローカル座標を計算
                    Vector3 offset = {
                        std::cos(orbitAngle_) * orbitRadius_,
                        std::sin(orbitAngle_ * 2.0f) * 0.5f + 1.0f, // 8の字にフワフワ
                        std::sin(orbitAngle_) * orbitRadius_
                    };
                    
                    transform->position_.x = targetTransform->position_.x + offset.x;
                    transform->position_.y = targetTransform->position_.y + offset.y;
                    transform->position_.z = targetTransform->position_.z + offset.z;
                }
            }
            break;
        }
        case DebrisState::BossOrbiting: {
            if (targetObject_) {
                auto targetTransform = targetObject_->GetComponent<TransformComponent>();
                if (targetTransform) {
                    float shieldRotationSpeed = 1.0f; // ボス側のパラメータを取得してもよいがとりあえず固定値
                    bossOrbitAngleX_ += bossOrbitSpeedX_ * shieldRotationSpeed;
                    bossOrbitAngleY_ += bossOrbitSpeedY_ * shieldRotationSpeed;
                    bossOrbitAngleZ_ += bossOrbitSpeedZ_ * shieldRotationSpeed;

                    Matrix4x4 rotMatrix = Math::MakeRotateXYZMatrix(Vector3{bossOrbitAngleX_, bossOrbitAngleY_, bossOrbitAngleZ_});
                    float bossShieldRadius = 8.0f; // 基本半径
                    Vector3 baseOffset = { 0, 0, bossShieldRadius + bossOrbitRadiusOffset_ };
                    Vector3 localPos = Math::TransformNormal(baseOffset, rotMatrix);

                    transform->position_.x = targetTransform->position_.x + localPos.x;
                    transform->position_.y = targetTransform->position_.y + localPos.y;
                    transform->position_.z = targetTransform->position_.z + localPos.z;

                    // 自身の回転も反映（2倍の速度で自転）
                    transform->rotation_ = { bossOrbitAngleX_ * 2.0f, bossOrbitAngleY_ * 2.0f, bossOrbitAngleZ_ * 2.0f };
                }
            }
            break;
        }
        case DebrisState::Thrown: {
            if (targetObject_ && targetObject_->GetIsActive()) {
                auto targetTransform = targetObject_->GetComponent<TransformComponent>();
                if (targetTransform) {
                    // 敵に向かって高速ホーミング移動
                    Vector3 diff = {
                        targetTransform->position_.x - transform->position_.x,
                        targetTransform->position_.y - transform->position_.y,
                        targetTransform->position_.z - transform->position_.z
                    };
                    // 正規化して一定速度で飛ばす
                    float len = std::sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
                    if (len > 0.001f) {
                        throwDirection_ = { diff.x / len, diff.y / len, diff.z / len };
                    }
                    
                    // 簡易ヒット判定
                    if (len < 1.0f) {
                        auto enemyComp = targetObject_->GetComponent<RailShooterEnemyComponent>();
                        if (enemyComp) {
                            enemyComp->TakeDamage(100);
                        }

                        auto bossComp = targetObject_->GetComponent<BossComponent>();
                        if (bossComp) {
                            bossComp->TakeDamage(10.0f);
                        }

                        // マネージャーが存在すればプールに正しく返却する
                        if (manager_) {
                            manager_->ReleaseDebris(gameObject_->shared_from_this());
                            // 仮想IDが割り当てられている場合は仮想データも破壊フラグを立てる
                            if (virtualId_ >= 0) {
                                manager_->NotifyDestroyed(virtualId_);
                            }
                        } else {
                            // フォールバック（通常は発生しない）
                            gameObject_->SetIsActive(false); 
                        }
                    }
                }
            }
            
            // ターゲットがない（または既に死んだ）場合でも、計算された(または初期設定された)方向に飛び続ける
            transform->position_.x += throwDirection_.x * throwSpeed_ * deltaTime;
            transform->position_.y += throwDirection_.y * throwSpeed_ * deltaTime;
            transform->position_.z += throwDirection_.z * throwSpeed_ * deltaTime;
            
            // TODO: 一定距離/時間で消滅させる等の処理が必要
            break;
        }
    }
}
