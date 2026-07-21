#include "DroneManagerComponent.h"
#include "DroneComponent.h"
#include "Framework/GameObject.h"
#include "Engine/Core/Math/MathFunction.h"

DroneManagerComponent::DroneManagerComponent() {}

void DroneManagerComponent::Initialize() {
}

void DroneManagerComponent::Start() {
}

void DroneManagerComponent::Update() {
    // 非アクティブになったドローンを回収する処理など
}

void DroneManagerComponent::OnRegisterProperties() {
}

void DroneManagerComponent::DeployDrones(std::weak_ptr<GameObject> boss, int count, BossBulletManagerComponent* bulletMgr) {
    if (!dronePool_) {
        auto factory = [this]() {
            auto obj = GetGameObject()->Instantiate("resources/prefabs/BossDrone.json");
            if (obj) {
                obj->SetIsActive(false);
            }
            return obj;
        };
        dronePool_ = std::make_unique<ObjectPool<GameObject>>(maxDrones_, factory);
    }
    if (!dronePool_) return;

    float angleStep = (Math::PI * 2.0f) / count;
    float baseRadius = 15.0f;
    float orbitSpeed = 1.0f;

    for (int i = 0; i < count; ++i) {
        auto handle = dronePool_->Acquire();
        if (handle.IsValid()) {
            GameObject* droneObj = dronePool_->Resolve(handle).get();
            if (droneObj) {
                droneObj->SetIsActive(true);
                activeDrones_.push_back(droneObj);

                auto droneComp = droneObj->GetComponent<DroneComponent>();
                if (!droneComp) {
                    droneComp = droneObj->AddComponent<DroneComponent>().get();
                }

                if (droneComp) {
                    droneComp->SetOrbit(boss, baseRadius, angleStep * i, orbitSpeed);
                    droneComp->SetBulletManager(bulletMgr);
                }
            }
        }
    }
}

void DroneManagerComponent::RecallAllDrones() {
    if (!dronePool_) return;
    for (auto* droneObj : activeDrones_) {
        if (droneObj) {
            droneObj->SetIsActive(false);
            // プールへの返却処理
        }
    }
    activeDrones_.clear();
}
