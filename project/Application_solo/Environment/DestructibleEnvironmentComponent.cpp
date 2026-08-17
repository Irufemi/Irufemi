#include "Environment/DestructibleEnvironmentComponent.h"
#include "Environment/DebrisManagerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Effect/EffectMaskComponent.h"
#include "Core/Math/Random/Random.h"
#include "Core/Utility/Log.h"
#include "Effects/EffectManagerComponent.h"
#include "Environment/DebrisComponent.h"
#include <iostream>

void DestructibleEnvironmentComponent::Start() {
    if (!gameObject_) return;
    
    auto scene = gameObject_->GetScene();
    if (scene) {
        auto managerObj = scene->FindGameObject("DebrisManager");
        if (managerObj) {
            debrisManager_ = managerObj->GetComponent<DebrisManagerComponent>();
        }
    }
}

void DestructibleEnvironmentComponent::TakeDamage(int damage) {
    if (hp_ <= 0) return; // 既に破壊されている

    hp_ -= damage;
    
    if (hp_ <= 0) {
        // 破壊イベント発火
        Log::OutPutLog(std::cout, "[DestructibleEnv] Environment Destroyed: " + gameObject_->GetName() + "\n");
        
        auto transform = gameObject_->GetTransform();
        if (transform && debrisManager_) {
            Irufemi::Vector3 pos = transform->GetWorldPosition();
            
            // 破壊エフェクト（ヒットエフェクト流用）
            if (auto effectManager = EffectManagerComponent::GetInstance()) {
                effectManager->PlayEffect("Hit", pos);
            }
            
            // 瓦礫のスポーン（散らばるように）
            for (int i = 0; i < debrisSpawnCount_; ++i) {
                auto debris = debrisManager_->GetDebris();
                if (debris) {
                    if (auto debrisTransform = debris->GetTransform()) {
                        // ランダムなオフセットと上方向への位置調整
                        Irufemi::Vector3 offset = {
                            Irufemi::Random::GeneratorFloat(-2.0f, 2.0f),
                            Irufemi::Random::GeneratorFloat(2.0f, 6.0f),
                            Irufemi::Random::GeneratorFloat(-2.0f, 2.0f)
                        };
                        debrisTransform->SetPosition(pos + offset);
                    }
                    
                    if (auto comp = debris->GetComponent<DebrisComponent>()) {
                        comp->SetState(DebrisState::Idle); // TODO: 空中にばらまく物理挙動を入れる場合はここで処理
                        comp->SetTarget(std::weak_ptr<GameObject>());
                    }
                }
            }
        }
        
        // 自身を非アクティブ化して消滅
        gameObject_->SetIsActive(false);
    }
}
