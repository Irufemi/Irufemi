#include "BossComponent.h"
#include "BossStateIdle.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "../DebrisManagerComponent.h"
#include "../DebrisComponent.h"
#include "../EnemyBeamComponent.h"
#include "../DroneManagerComponent.h"
#include "../BossBulletManagerComponent.h"
#include "../TargetableComponent.h"
#include <algorithm>
#include <iostream>

BossComponent::BossComponent() {
}

void BossComponent::Initialize() {
    if (!gameObject_->GetComponent<TargetableComponent>()) {
        gameObject_->AddComponent<TargetableComponent>();
    }
    hp_ = maxHp_;
    isShieldsInitialized_ = false;

    if (gameObject_) {
        auto collider = gameObject_->GetComponent<SphereColliderComponent>();
        if (!collider) {
            collider = gameObject_->AddComponent<SphereColliderComponent>().get();
        }
        if (collider) {
            collider->isTrigger_ = true;
        }
    }

    if (gameObject_) {
        beamComponent_ = gameObject_->GetComponent<EnemyBeamComponent>();
        if (!beamComponent_) {
            auto comp = gameObject_->AddComponent<EnemyBeamComponent>();
            beamComponent_ = comp.get();
            beamComponent_->Initialize();
        }
    }
    beamTimer_ = 0.0f;

    // 初期ステートの設定
    ChangeState(std::make_unique<BossStateIdle>());
}

void BossComponent::Start() {
    if (!gameObject_) return;
    auto scene = gameObject_->GetScene();
    if (scene) {
        auto container = scene->FindGameObject("BossContainer");
        if (container) {
            bossContainer_ = container;
            auto droneObj = scene->FindGameObject("BossDroneManager");
            if (droneObj) droneManager_ = droneObj->GetComponent<DroneManagerComponent>();
            auto bulletObj = scene->FindGameObject("BossBulletManager");
            if (bulletObj) bulletManager_ = bulletObj->GetComponent<BossBulletManagerComponent>();
        }

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

                for (int i = 0; i < maxShieldCount_; ++i) {
                    auto debrisObj = debrisManager_->GetDebris();
                    if (debrisObj) {
                        setupDebris(debrisObj);
                        initialShieldsSpawned_++;
                    }
                }
                isShieldsInitialized_ = true;
                
                if (droneManager_ && bulletManager_) {
                    droneManager_->DeployDrones(gameObject_->shared_from_this(), 10, bulletManager_);
                }
            }
        }
    }
}

void BossComponent::Update() {
    if (!gameObject_) return;
    
    if (currentState_) {
        currentState_->Update(this);
    }
}

void BossComponent::OnRegisterProperties() {
    RegisterProperty("Max HP", &maxHp_);
    RegisterProperty("Max Shield Count", &maxShieldCount_);
    RegisterProperty("Shield Radius", &shieldRadius_);
    RegisterProperty("Beam Interval", &beamInterval_);
    RegisterProperty("Beam Offset Z", &beamOffsetZ_);
    RegisterProperty("Beam Offset Y", &beamOffsetY_);
    RegisterProperty("Beam Range", &beamRange_);
}

std::shared_ptr<GameObject> BossComponent::ExtractDebris() {
    if (shields_.empty()) return nullptr;
    
    auto debris = shields_.back();
    shields_.pop_back();
    
    if (debris) {
        auto debrisComp = debris->GetComponent<DebrisComponent>();
        if (debrisComp) {
            debrisComp->SetState(DebrisState::Idle);
            debrisComp->SetTarget(std::weak_ptr<GameObject>());
        }
    }
    return debris;
}

void BossComponent::RemoveShield(std::shared_ptr<GameObject> shield) {
    auto it = std::find(shields_.begin(), shields_.end(), shield);
    if (it != shields_.end()) {
        shields_.erase(it);
    }
}

void BossComponent::TakeDamage(float damage) {
    if (currentState_) {
        currentState_->OnTakeDamage(this, damage);
    }
}

void BossComponent::ChangeState(std::unique_ptr<IBossState> newState) {
    if (currentState_) {
        currentState_->Exit(this);
    }
    currentState_ = std::move(newState);
    if (currentState_) {
        currentState_->Enter(this);
    }
}
