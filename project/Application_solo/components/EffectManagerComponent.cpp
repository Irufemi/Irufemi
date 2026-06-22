#include "EffectManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Effect/ParticleEmitterComponent.h"
#include "Framework/Component/Utility/LifetimeComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"

EffectManagerComponent* EffectManagerComponent::instance_ = nullptr;

void EffectManagerComponent::OnRegisterProperties() {
    RegisterProperty("Hit Effect Path", &hitEffectPath_);
}

void EffectManagerComponent::Initialize() {
    instance_ = this;
    effectDictionary_["Hit"] = hitEffectPath_;
}

void EffectManagerComponent::Start() {
    hitEffectPool_ = std::make_unique<ObjectPool<GameObject>>(maxHitEffects_, [this]() {
        auto obj = gameObject_->Instantiate(hitEffectPath_);
        if (obj) {
            obj->SetIsActive(false);
            
            // 寿命コンポーネントがあれば、プール運用のためDestroyではなくDisableに変更する
            if (auto lifetime = obj->GetComponent<LifetimeComponent>()) {
                lifetime->SetTimeoutAction(TimeoutAction::Disable);
            }
        }
        return obj;
    });
}

void EffectManagerComponent::Update() {
    for (auto it = activeEffects_.begin(); it != activeEffects_.end(); ) {
        // オブジェクトが非アクティブになっていれば、寿命（または自律的終了）を迎えたとみなしてプールに返却
        if (!it->obj->GetIsActive()) {
            if (hitEffectPool_) {
                hitEffectPool_->Release(it->obj);
            }
            it = activeEffects_.erase(it);
        } else {
            ++it;
        }
    }
}

void EffectManagerComponent::PlayEffect(const std::string& effectKey, const Vector3& worldPosition) {
    if (!gameObject_) return;

    auto it = effectDictionary_.find(effectKey);
    if (it == effectDictionary_.end() || it->second.empty()) return;

    if (effectKey == "Hit" && hitEffectPool_) {
        auto obj = hitEffectPool_->Acquire();
        if (obj) {
            if (auto t = obj->GetComponent<TransformComponent>()) {
                t->SetPosition(worldPosition);
            }
            // アクティブ化して LifetimeComponent のタイマーをリセットする
            obj->SetIsActive(true);
            if (auto lifetime = obj->GetComponent<LifetimeComponent>()) {
                lifetime->Initialize();
            }
            
            // ツリー全体からすべての ParticleEmitterComponent を取得して再発火させる
            auto emitters = obj->GetComponentsInChildren<ParticleEmitterComponent>();
            for (auto pe : emitters) {
                pe->Restart(false);
            }
            
            activeEffects_.push_back({obj, 0.0f}); // timerはもう使わないが構造体互換のため0をセット
        }
    }
}
