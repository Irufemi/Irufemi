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
            
            // プール運用の場合、自身をDestroyするLifetimeComponentがあるとシーンから除外されてしまうため削除する
            if (auto lifetime = obj->GetComponent<LifetimeComponent>()) {
                obj->RemoveComponent(lifetime);
            }
        }
        return obj;
    });
}

void EffectManagerComponent::Update() {
    float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    for (auto it = activeEffects_.begin(); it != activeEffects_.end(); ) {
        it->timer -= deltaTime;
        if (it->timer <= 0.0f) {
            it->obj->SetIsActive(false);
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
            obj->SetIsActive(true);
            
            // ルートおよびすべての子オブジェクトの ParticleEmitterComponent を再生する
            auto playEmitters = [](auto& self, std::shared_ptr<GameObject> current) -> void {
                if (!current) return;
                if (auto pe = current->GetComponent<ParticleEmitterComponent>()) {
                    pe->Restart();
                }
                for (auto& child : current->GetChildren()) {
                    self(self, child);
                }
            };
            playEmitters(playEmitters, obj);
            
            activeEffects_.push_back({obj, effectDuration_});
        }
    }
}
