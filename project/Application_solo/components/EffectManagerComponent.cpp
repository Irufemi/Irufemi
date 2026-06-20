#include "EffectManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "HitEffectComponent.h"

EffectManagerComponent* EffectManagerComponent::instance_ = nullptr;

void EffectManagerComponent::OnRegisterProperties() {
    RegisterProperty("Hit Effect Path", &hitEffectPath_);
    RegisterHeader("Hit Effect Override");
    RegisterProperty("Show Debug Area", &showHitDebugArea_);
    RegisterProperty("Core Flash Burst", &hitCoreFlashBurst_);
    RegisterProperty("Sparks Burst", &hitSparksBurst_);
    RegisterProperty("Shockwave Burst", &hitShockwaveBurst_);
}

void EffectManagerComponent::Initialize() {
    instance_ = this;
    
    // 辞書を初期化
    effectDictionary_["Hit"] = hitEffectPath_;
}

void EffectManagerComponent::PlayEffect(const std::string& effectKey, const Vector3& worldPosition) {
    if (!gameObject_) return;

    auto it = effectDictionary_.find(effectKey);
    if (it == effectDictionary_.end() || it->second.empty()) return;

    // TODO: ここでObject Poolから取得するように変更する
    // 現状は都度 Instantiate する
    auto spawnedObj = gameObject_->Instantiate(it->second, worldPosition);
    if (spawnedObj) {
        // HitEffectComponent がアタッチされていれば、PlayAt を呼び出して再生を初期化する
        if (auto hitEffect = spawnedObj->GetComponent<HitEffectComponent>()) {
            if (effectKey == "Hit") {
                hitEffect->SetParameters(hitCoreFlashBurst_, hitSparksBurst_, hitShockwaveBurst_, showHitDebugArea_);
            }
            hitEffect->PlayAt(worldPosition);
        }
    }
}
