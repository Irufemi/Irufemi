#include "Combat/Boss/BossComponent.h"
#include "Combat/Boss/BossStateIdle.h"
#include "Combat/BossBulletManagerComponent.h"
#include "Combat/DroneManagerComponent.h"
#include "Combat/EnemyBeamComponent.h"
#include "Core/Utility/Log.h"
#include "Environment/DebrisComponent.h"
#include "Environment/DebrisManagerComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Player/TargetableComponent.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

void BossComponent::LoadStatusFromJson() {
    if (statusDataPath_.empty())
        return;

    std::ifstream file(statusDataPath_);
    if (!file.is_open()) {
        Log::OutPutLog(std::cout, "[BossComponent] Failed to load status: " + statusDataPath_ + "\n");
        return;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (j.contains("maxHp")) {
            maxHp_ = j["maxHp"].get<float>();
            hp_ = maxHp_;
        }
        if (j.contains("maxShieldCount")) {
            maxShieldCount_ = j["maxShieldCount"].get<int>();
        }
        if (j.contains("shieldRadius")) {
            shieldRadius_ = j["shieldRadius"].get<float>();
        }
        if (j.contains("beamInterval")) {
            beamInterval_ = j["beamInterval"].get<float>();
        }
        if (j.contains("beamRange")) {
            beamRange_ = j["beamRange"].get<float>();
        }
    } catch (const std::exception& e) {
        Log::OutPutLog(std::cout, std::string("[BossComponent] JSON Parse Error: ") + e.what() + "\n");
    }
}

BossComponent::BossComponent() {}

void BossComponent::Initialize() {
    if (!gameObject_->GetComponent<TargetableComponent>()) {
        gameObject_->AddComponent<TargetableComponent>();
    }

    LoadStatusFromJson();
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
    if (!gameObject_)
        return;
    auto scene = gameObject_->GetScene();
    if (scene) {
        auto container = scene->FindGameObject("BossContainer");
        if (container) {
            bossContainer_ = container;
            auto droneObj = scene->FindGameObject("BossDroneManager");
            if (droneObj)
                droneManager_ = droneObj->GetComponent<DroneManagerComponent>();
            auto bulletObj = scene->FindGameObject("BossBulletManager");
            if (bulletObj)
                bulletManager_ = bulletObj->GetComponent<BossBulletManagerComponent>();
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
    if (!gameObject_)
        return;

    if (currentState_) {
        currentState_->Update(this);
    }
}

void BossComponent::OnRegisterProperties() {
    RegisterProperty("Status Data Path", &statusDataPath_);
    RegisterProperty("Beam Offset Z", &beamOffsetZ_);
    RegisterProperty("Beam Offset Y", &beamOffsetY_);
}

std::shared_ptr<GameObject> BossComponent::ExtractDebris() {
    if (shields_.empty())
        return nullptr;

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
