#include "EffectManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"

EffectManagerComponent* EffectManagerComponent::instance_ = nullptr;

void EffectManagerComponent::OnRegisterProperties() {
    RegisterProperty("Hit Effect Path", &hitEffectPath_);
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
    // 現状は都度 Instantiate する。生成後はプレハブ側のアタッチされたコンポーネント（ParticleEmitterのAwake等）により自動再生される想定。
    gameObject_->Instantiate(it->second, worldPosition);
}
