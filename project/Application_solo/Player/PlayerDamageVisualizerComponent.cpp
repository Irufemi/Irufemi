#include "Player/PlayerDamageVisualizerComponent.h"
#include "Player/PlayerHealthComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/SkinnedMeshRendererComponent.h"
#include "Framework/Component/Effect/ScreenEffectComponent.h"
#include "Framework/Component/Camera/CameraShakeComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Core/System/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Core/Utility/Log.h"
#include <cmath>
#include <iostream>

void PlayerDamageVisualizerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    RegisterProperty("Flash Interval", &flashInterval_);
    RegisterProperty("Flash Duration", &flashDuration_);
    RegisterProperty("Shake Intensity", &shakeIntensity_);
}

void PlayerDamageVisualizerComponent::Initialize() {
    flashTimer_ = 0.0f;
    flashRemaining_ = 0.0f;
    isFlashing_ = false;
    colorCached_ = false;
}

void PlayerDamageVisualizerComponent::Start() {
    if (!gameObject_) {
        return;
    }

    healthComp_ = gameObject_->GetComponent<PlayerHealthComponent>();
    if (healthComp_) {
        // 被弾イベントリスナーを登録
        healthComp_->AddOnDamageTakenListener([this](int damage) {
            (void)damage;
            TriggerDamageFlash();
            TriggerCameraShake();
            TriggerScreenEffect();
        });

        // 死亡イベントリスナーを登録
        healthComp_->AddOnPlayerDiedListener([this]() { TriggerDeathVisuals(); });
    }
}

BaseModel* PlayerDamageVisualizerComponent::GetTargetModel() {
    if (!gameObject_) {
        return nullptr;
    }

    // dynamic_cast による型安全なモデルポインタ取得
    if (auto mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
        if (auto renderable = mesh->GetRenderable()) {
            return dynamic_cast<BaseModel*>(renderable);
        }
    } else if (auto skinned = gameObject_->GetComponent<SkinnedMeshRendererComponent>()) {
        if (auto renderable = skinned->GetRenderable()) {
            return dynamic_cast<BaseModel*>(renderable);
        }
    }

    return nullptr;
}

void PlayerDamageVisualizerComponent::TriggerDamageFlash() {
    if (healthComp_) {
        flashRemaining_ = healthComp_->GetMaxInvincibilityTime();
    } else {
        flashRemaining_ = flashDuration_;
    }

    isFlashing_ = true;
    flashTimer_ = 0.0f;

    BaseModel* model = GetTargetModel();
    if (model && !colorCached_) {
        originalBaseColor_ = model->GetColor();
        colorCached_ = true;
    }
}

void PlayerDamageVisualizerComponent::TriggerCameraShake() {
    if (!gameObject_) {
        return;
    }
    if (auto scene = gameObject_->GetScene()) {
        if (auto mainCameraObj = scene->FindGameObject("MainCamera")) {
            if (auto shakeComp = mainCameraObj->GetComponent<CameraShakeComponent>()) {
                shakeComp->PlayShake(shakeIntensity_, shakeFrames_, shakeFrequency_);
            }
        }
    }
}

void PlayerDamageVisualizerComponent::TriggerScreenEffect() {
    if (!gameObject_) {
        return;
    }
    for (auto& comp : gameObject_->GetComponents()) {
        if (auto screenEffect = std::dynamic_pointer_cast<ScreenEffectComponent>(comp)) {
            screenEffect->Play();
        }
    }
}

void PlayerDamageVisualizerComponent::TriggerDeathVisuals() {
    if (!gameObject_) {
        return;
    }

    // 死亡時に自機モデルを非表示
    if (auto mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
        mesh->SetVisible(false);
    } else if (auto skinned = gameObject_->GetComponent<SkinnedMeshRendererComponent>()) {
        skinned->SetVisible(false);
    }

    // 点滅状態を解除し元の色に戻す
    if (isFlashing_) {
        isFlashing_ = false;
        if (BaseModel* model = GetTargetModel()) {
            if (colorCached_) {
                model->SetColor(originalBaseColor_);
            }
        }
    }
}

void PlayerDamageVisualizerComponent::Update() {
    float dt = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (dt <= 0.0f) {
        return;
    }

    // 点滅処理の更新
    if (isFlashing_) {
        flashRemaining_ -= dt;
        flashTimer_ += dt;

        BaseModel* model = GetTargetModel();
        if (model) {
            if (!colorCached_) {
                originalBaseColor_ = model->GetColor();
                colorCached_ = true;
            }

            if (fmod(flashTimer_, flashInterval_ * 2.0f) < flashInterval_) {
                model->SetColor(flashColor_);
            } else {
                model->SetColor(originalBaseColor_);
            }
        }

        if (flashRemaining_ <= 0.0f) {
            isFlashing_ = false;
            flashRemaining_ = 0.0f;
            if (model && colorCached_) {
                model->SetColor(originalBaseColor_);
            }
        }
    }
}
