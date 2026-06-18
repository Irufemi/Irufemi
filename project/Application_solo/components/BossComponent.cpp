#include "BossComponent.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "DebrisManagerComponent.h"
#include "DebrisComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include <Windows.h>
#include <string>

BossComponent::BossComponent() {
}

void BossComponent::Initialize() {
    hp_ = maxHp_;
    state_ = BossState::Idle;
    isShieldsInitialized_ = false;

    if (gameObject_) {
        auto collider = gameObject_->GetComponent<SphereColliderComponent>();
        if (!collider) {
            collider = gameObject_->AddComponent<SphereColliderComponent>().get();
            collider->isTrigger_ = true;
            collider->SetLocalRadius(5.0f);
        }
    }
}

void BossComponent::Update() {
    if (!gameObject_) return;
    
    // DebrisManagerから初回だけシールド用のガレキを取得する
    if (!isShieldsInitialized_) {
        auto scene = gameObject_->GetScene();
        if (scene) {
            auto managerObj = scene->FindGameObject("DebrisManager");
            if (managerObj) {
                debrisManager_ = managerObj->GetComponent<DebrisManagerComponent>();
                if (debrisManager_) {
                    auto setupDebris = [this](std::shared_ptr<GameObject> debrisObj) {
                        auto debrisComp = debrisObj->GetComponent<DebrisComponent>();
                        if (debrisComp) {
                            debrisComp->SetTarget(gameObject_->shared_from_this());
                            debrisComp->SetState(DebrisState::BossOrbiting); 
                        }
                        shields_.push_back(debrisObj);
                    };

                    // 残りを取得してセットアップ
                    for (int i = 0; i < maxShieldCount_; ++i) {
                        auto debrisObj = debrisManager_->AcquireDebris();
                        if (debrisObj) {
                            setupDebris(debrisObj);
                        }
                    }
                    isShieldsInitialized_ = true;
                }
            }
        }
    }
    
    // CoreExposed への遷移チェック
    if (state_ == BossState::Idle && shields_.empty() && isShieldsInitialized_) {
        state_ = BossState::CoreExposed;
    }
}

void BossComponent::OnRegisterProperties() {
    RegisterProperty("Max HP", &maxHp_);
    RegisterProperty("Max Shield Count", &maxShieldCount_);
    RegisterProperty("Shield Radius", &shieldRadius_);
}

std::shared_ptr<GameObject> BossComponent::ExtractDebris() {
    if (shields_.empty()) return nullptr;
    
    // シールドリストから1つ取り出す
    auto debris = shields_.back();
    shields_.pop_back();
    
    // 取り出したガレキの状態は引っ張られる状態に変更する
    if (debris) {
        auto debrisComp = debris->GetComponent<DebrisComponent>();
        if (debrisComp) {
            debrisComp->SetState(DebrisState::Idle);
            debrisComp->SetTarget(std::weak_ptr<GameObject>());
        }
    }
    
    return debris;
}

void BossComponent::TakeDamage(float damage) {
    if (state_ == BossState::Destroyed) return;

    if (state_ == BossState::CoreExposed) {
        hp_ -= damage;
        
        std::string dmgLog = "Boss took damage! HP: " + std::to_string(hp_) + "\n";
        OutputDebugStringA(dmgLog.c_str());

        if (hp_ <= 0) {
            hp_ = 0;
            state_ = BossState::Destroyed;
            OutputDebugStringA("Boss Destroyed!\n");
        }
    } else {
        // シールドがある場合はダメージ無効、またはシールドが身代わりになる
        OutputDebugStringA("Boss blocked damage with shield!\n");
    }
}
