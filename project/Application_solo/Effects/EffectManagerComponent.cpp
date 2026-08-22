#include "Effects/EffectManagerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Effect/ParticleEmitterComponent.h"
#include "Framework/Component/Utility/LifetimeComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Framework/Scene/BaseScene.h"
#include "Core/Utility/Log.h"
#include <iostream>

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
        auto obj = gameObject_->Instantiate(hitEffectPath_); // ☛Instantiate内部でシーン登録される
        if (obj) {
            obj->SetIsActive(false); // Removeせずに非アクティブ状態で休眠させる
            
            // 寿命コンポーネントがあれば、プール運用のためにDestroyではなくDisableに変更する
            if (auto lifetime = obj->GetComponent<LifetimeComponent>()) {
                lifetime->SetTimeoutAction(TimeoutAction::Disable);
            }
        }
        return obj;
    });
}

void EffectManagerComponent::Update() {
    for (auto it = activeEffects_.begin(); it != activeEffects_.end(); ) {
        if (hitEffectPool_) {
            auto obj = hitEffectPool_->Resolve(it->handle);
            // オブジェクトが非アクティブになっていれば、寿命（または自律的終了）を迎えたとみなしてプールに返却
            if (obj && !obj->GetIsActive()) {
                hitEffectPool_->Release(it->handle);
                it = activeEffects_.erase(it);
                
                continue;
            }
        }
        ++it;
    }
}

void EffectManagerComponent::PlayEffect(const std::string& effectKey, const Irufemi::Vector3& worldPosition) {
    if (!gameObject_) return;

    auto it = effectDictionary_.find(effectKey);
    if (it == effectDictionary_.end() || it->second.empty()) return;

    if (effectKey == "Hit" && hitEffectPool_) {
        auto handle = hitEffectPool_->Acquire();
        
        // プールが枯渇した場合、一番古いエフェクトを強制終了して再利用する
        if (!handle.IsValid() && !activeEffects_.empty()) {
            auto oldest = activeEffects_.front();
            activeEffects_.erase(activeEffects_.begin());
            
            auto obj = hitEffectPool_->Resolve(oldest.handle);
            if (obj) {
                obj->SetIsActive(false);
            }
            hitEffectPool_->Release(oldest.handle);
            handle = hitEffectPool_->Acquire();
        }
        
        if (handle.IsValid()) {
            auto obj = hitEffectPool_->Resolve(handle);
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
                
                activeEffects_.push_back({handle, 0.0f}); // timerはもう使わないが構造体互換のため0をセット
            }
        }
    }
}
