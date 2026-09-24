#include "Combat/Boss/BossDamageVisualizerComponent.h"
#include "Combat/Boss/BossComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Framework/Component/Camera/CameraShakeComponent.h"

void BossDamageVisualizerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    RegisterProperty("Damage Shake Intensity", &damageShakeIntensity_);
    RegisterProperty("Damage Shake Frames", &damageShakeFrames_);
    RegisterProperty("Damage Shake Frequency", &damageShakeFrequency_);
    RegisterProperty("Death Shake Intensity", &deathShakeIntensity_);
    RegisterProperty("Death Shake Frames", &deathShakeFrames_);
    RegisterProperty("Death Shake Frequency", &deathShakeFrequency_);
}

void BossDamageVisualizerComponent::Initialize() {
    mainCameraObj_.reset();
}

void BossDamageVisualizerComponent::Start() {
    if (!gameObject_) {
        return;
    }

    bossComp_ = gameObject_->GetComponent<BossComponent>();
    if (bossComp_) {
        std::weak_ptr<GameObject> weakObj = gameObject_->shared_from_this();

        // 被弾イベントリスナーを登録
        bossComp_->AddOnDamageTakenListener([weakObj](float damage) {
            if (auto obj = weakObj.lock()) {
                if (auto visualizer = obj->GetComponent<BossDamageVisualizerComponent>()) {
                    visualizer->TriggerDamageShake(damage);
                }
            }
        });

        // 撃破イベントリスナーを登録
        bossComp_->AddOnBossDiedListener([weakObj]() {
            if (auto obj = weakObj.lock()) {
                if (auto visualizer = obj->GetComponent<BossDamageVisualizerComponent>()) {
                    visualizer->TriggerDeathShake();
                }
            }
        });
    }
}

std::shared_ptr<GameObject> BossDamageVisualizerComponent::GetMainCamera() const {
    if (auto cam = mainCameraObj_.lock()) {
        return cam;
    }
    if (gameObject_) {
        if (auto scene = gameObject_->GetScene()) {
            if (auto cam = scene->FindGameObject("MainCamera")) {
                mainCameraObj_ = cam;
                return cam;
            }
        }
    }
    return nullptr;
}

CameraShakeComponent* BossDamageVisualizerComponent::GetCameraShake() const {
    if (auto cam = GetMainCamera()) {
        return cam->GetComponent<CameraShakeComponent>();
    }
    return nullptr;
}

void BossDamageVisualizerComponent::TriggerDamageShake(float damage) {
    (void)damage;
    if (auto shake = GetCameraShake()) {
        shake->PlayShake(damageShakeIntensity_, damageShakeFrames_, damageShakeFrequency_);
    }
}

void BossDamageVisualizerComponent::TriggerDeathShake() {
    if (auto shake = GetCameraShake()) {
        shake->PlayShake(deathShakeIntensity_, deathShakeFrames_, deathShakeFrequency_);
    }
}

bool BossDamageVisualizerComponent::IsDeathShakePlaying() const {
    if (auto shake = GetCameraShake()) {
        return shake->IsPlaying();
    }
    return false;
}
