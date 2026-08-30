#include "Effects/EffectManagerComponent.h"
#include <Windows.h>
#include <cstdio>
#include <iostream>
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Effect/ParticleEmitterComponent.h"
#include "Framework/Component/Utility/LifetimeComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Framework/Scene/BaseScene.h"
#include "Core/Utility/Log.h"
#include <iostream>


void EffectManagerComponent::OnRegisterProperties() {
    RegisterProperty("Hit Effect Path", &hitEffectPath_);
    RegisterProperty("Dust Effect Path", &dustEffectPath_);
}

void EffectManagerComponent::Initialize() {
    effectDictionary_["Hit"] = hitEffectPath_;
    effectDictionary_["Dust"] = dustEffectPath_;
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

    dustEffectPool_ = std::make_unique<ObjectPool<GameObject>>(maxDustEffects_, [this]() {
        auto obj = gameObject_->Instantiate(dustEffectPath_); // ☛Instantiate内部でシーン登録される
        if (obj) {
            obj->SetIsActive(false); // Removeせずに非アクティブ状態で休眠させる
            
            if (auto lifetime = obj->GetComponent<LifetimeComponent>()) {
                lifetime->SetTimeoutAction(TimeoutAction::Disable);
            }
        }
        return obj;
    });
}

void EffectManagerComponent::Update() {
    for (auto it = activeEffects_.begin(); it != activeEffects_.end(); ) {
        bool handled = false;
        
        if (it->effectKey == "Hit" && hitEffectPool_) {
            auto obj = hitEffectPool_->Resolve(it->handle);
            if (obj && !obj->GetIsActive()) {
                hitEffectPool_->Release(it->handle);
                it = activeEffects_.erase(it);
                continue;
            }
            handled = true;
        } else if (it->effectKey == "Dust" && dustEffectPool_) {
            auto obj = dustEffectPool_->Resolve(it->handle);
            if (obj && !obj->GetIsActive()) {
                dustEffectPool_->Release(it->handle);
                it = activeEffects_.erase(it);
                continue;
            }
            handled = true;
        }
        
        if (!handled) {
            // プールがないか不明なエフェクト
            it = activeEffects_.erase(it);
            continue;
        }
        
        ++it;
    }
}

void EffectManagerComponent::PlayEffect(const std::string& effectKey, const Irufemi::Vector3& worldPosition) {
    if (!gameObject_) return;

    auto it = effectDictionary_.find(effectKey);
    if (it == effectDictionary_.end() || it->second.empty()) return;

    ObjectPool<GameObject>* targetPool = nullptr;
    if (effectKey == "Hit") targetPool = hitEffectPool_.get();
    else if (effectKey == "Dust") targetPool = dustEffectPool_.get();

    if (targetPool) {
        auto handle = targetPool->Acquire();
        
        // プールが枯渇した場合、一番古いエフェクトを強制終了して再利用する
        if (!handle.IsValid() && !activeEffects_.empty()) {
            Log::OutPutLog(std::cout, "[EffectManager] Pool exhausted. Reusing oldest effect.\n");
            
            // 最も古い同じ種類のエフェクトを探す
            auto oldestIt = activeEffects_.begin();
            while (oldestIt != activeEffects_.end() && oldestIt->effectKey != effectKey) {
                ++oldestIt;
            }
            
            if (oldestIt != activeEffects_.end()) {
                auto obj = targetPool->Resolve(oldestIt->handle);
                if (obj) {
                    obj->SetIsActive(false);
                }
                targetPool->Release(oldestIt->handle);
                activeEffects_.erase(oldestIt);
                handle = targetPool->Acquire();
            }
        }
        
        if (handle.IsValid()) {
            auto obj = targetPool->Resolve(handle);
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
                Log::OutPutLog(std::cout, "[EffectManager] Found " + std::to_string(emitters.size()) + " emitters for " + effectKey + " effect. Position: " + std::to_string(worldPosition.x) + ", " + std::to_string(worldPosition.y) + ", " + std::to_string(worldPosition.z) + "\n");
                
                for (auto pe : emitters) {
                    pe->Restart(false);
                }
                
                activeEffects_.push_back({handle, 0.0f, effectKey}); // timerはもう使わないが構造体互換のため0をセット
            }
        }
    }
}
