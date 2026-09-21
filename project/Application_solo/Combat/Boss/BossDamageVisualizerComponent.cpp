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
    mainCameraShakeComp_ = nullptr;
}

void BossDamageVisualizerComponent::Start() {
    if (!gameObject_) {
        return;
    }

    bossComp_ = gameObject_->GetComponent<BossComponent>();
    if (bossComp_) {
        // 被弾イベントリスナーを登録
        bossComp_->AddOnDamageTakenListener([this](float damage) { TriggerDamageShake(damage); });

        // 撃破イベントリスナーを登録
        bossComp_->AddOnBossDiedListener([this]() { TriggerDeathShake(); });
    }
}

std::shared_ptr<GameObject> BossDamageVisualizerComponent::GetMainCamera() {
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

CameraShakeComponent* BossDamageVisualizerComponent::GetCameraShake() {
    auto cam = mainCameraObj_.lock();
    if (!cam) {
        mainCameraShakeComp_ = nullptr;
        cam = GetMainCamera();
    }
    if (cam) {
        if (!mainCameraShakeComp_) {
            mainCameraShakeComp_ = cam->GetComponent<CameraShakeComponent>();
        }
        return mainCameraShakeComp_;
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

bool BossDamageVisualizerComponent::IsDeathShakePlaying() {
    if (auto shake = GetCameraShake()) {
        return shake->IsPlaying();
    }
    return false;
}
