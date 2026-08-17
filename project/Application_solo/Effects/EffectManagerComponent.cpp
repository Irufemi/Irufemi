#include "Effects/EffectManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Effect/ParticleEmitterComponent.h"
#include "Framework/Component/Utility/LifetimeComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Framework/BaseScene.h"

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
        auto obj = gameObject_->Instantiate(hitEffectPath_); // ★ Instantiate内部でシーン登録されるが、すぐ後で外せばよい
        if (obj) {
            obj->SetIsActive(false);
            if (auto scene = gameObject_->GetScene()) {
                scene->RemoveGameObject(obj);
            }
            
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
        if (hitEffectPool_) {
            auto obj = hitEffectPool_->Resolve(it->handle);
            // オブジェクトが非アクティブになっていれば、寿命（または自律的終了）を迎えたとみなしてプールに返却
            if (obj && !obj->GetIsActive()) {
                hitEffectPool_->Release(it->handle);
                it = activeEffects_.erase(it);
                
                // シーンから外す
                if (auto scene = gameObject_->GetScene()) {
                    scene->RemoveGameObject(obj);
                }
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
        if (handle.IsValid()) {
            auto obj = hitEffectPool_->Resolve(handle);
            if (obj) {
                if (auto t = obj->GetComponent<TransformComponent>()) {
                    t->SetPosition(worldPosition);
                }
                
                // シーンに追加する
                if (auto scene = gameObject_->GetScene()) {
                    scene->AddGameObject(obj);
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
