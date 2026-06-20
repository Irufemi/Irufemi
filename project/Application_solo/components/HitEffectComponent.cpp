#include "HitEffectComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Effect/ParticleEmitterComponent.h"

void HitEffectComponent::OnRegisterProperties() {
    RegisterProperty("Core Flash Burst", &coreFlashBurstCount_);
    RegisterProperty("Sparks Burst", &sparksBurstCount_);
    RegisterProperty("Shockwave Burst", &shockwaveBurstCount_);
    RegisterProperty("Show Debug Area", &showDebugArea_);
}

void HitEffectComponent::PlayAt(const Vector3& pos) {
    if (!gameObject_) return;

    if (auto transform = gameObject_->GetComponent<TransformComponent>()) {
        transform->SetPosition(pos);
    }

    // 各子オブジェクトから ParticleEmitterComponent を探し、バーストを発火させる
    for (auto& child : gameObject_->GetChildren()) {
        if (!child) continue;
        
        if (auto emitter = child->GetComponent<ParticleEmitterComponent>()) {
            // 子オブジェクトの名前（またはタグ）に応じてバースト数を変える
            if (child->GetName() == "CoreFlash") {
                emitter->EmitBurst(coreFlashBurstCount_);
            } else if (child->GetName() == "Sparks") {
                emitter->EmitBurst(sparksBurstCount_);
            } else if (child->GetName() == "Shockwave") {
                emitter->EmitBurst(shockwaveBurstCount_);
            } else {
                // デフォルト（念のため）
                emitter->EmitBurst(1);
            }
            // 親のフラグで子のデバッグ表示を一括制御
            if (auto pObj = emitter->GetParticleObject()) {
                pObj->SetShowDebugArea(showDebugArea_);
            }
        }
    }
}
