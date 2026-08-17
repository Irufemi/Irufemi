#include "Combat/BossBulletManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/VirtualEntity/VirtualEntityManagerComponent.h"
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Player/GravityPlayerComponent.h"
#include "Engine/Manager/CollisionManager.h"
#include "Framework/Component/Collider/ColliderComponent.h"
#include "Engine/Core/Utility/Log.h"
#include <iostream>

BossBulletManagerComponent::BossBulletManagerComponent() {}

void BossBulletManagerComponent::Initialize() {
    virtualManager_ = gameObject_->GetComponent<VirtualEntityManagerComponent>();
    if (!virtualManager_) {
        virtualManager_ = gameObject_->AddComponent<VirtualEntityManagerComponent>().get();
    }
    
    // バッチレンダラに球モデルを設定
    if (auto batchRenderer = gameObject_->GetComponent<ModelBatchRendererComponent>()) {
        batchRenderer->LoadModel("resources/model/BossBulletSphere.obj");
        // 紫色などはマテリアル側で設定する必要がありますが、ここでは一旦モデルを描画します
    }

    auto factory = []() -> std::shared_ptr<GameObject> { return nullptr; };
    virtualManager_->Setup(0, maxBullets_, factory);
    
    bulletDataList_.resize(maxBullets_);
    while (!activeVirtualIds_.empty()) activeVirtualIds_.pop();
}

void BossBulletManagerComponent::Start() {
}

void BossBulletManagerComponent::Update() {
    if (!virtualManager_) return;

    float dt = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (dt <= 0.0f) dt = 1.0f / 60.0f;

    auto& virtualInstances = virtualManager_->GetDenseInstances();
    
    int activeCount = static_cast<int>(activeVirtualIds_.size());
    for (int i = 0; i < activeCount; ++i) {
        int vid = activeVirtualIds_.front();
        activeVirtualIds_.pop();
        
        auto& data = bulletDataList_[vid];
        data.lifeTimer -= dt;
        
        if (data.lifeTimer <= 0.0f) {
            ReleaseBullet(vid);
            continue;
        }
        
        // Find index in dense_ array
        int denseIndex = virtualManager_->GetSparseIndex(vid);
        if (denseIndex >= 0) {
            auto& vi = virtualInstances[denseIndex];
            vi.position_ += data.velocity * dt;
            
            auto engine = BaseModel::GetIrufemiEngine();
            auto cm = engine->GetCollisionManager();
            if (cm) {
                Irufemi::Vector3 minPos = { vi.position_.x - hitRadius_, vi.position_.y - hitRadius_, vi.position_.z - hitRadius_ };
                Irufemi::Vector3 maxPos = { vi.position_.x + hitRadius_, vi.position_.y + hitRadius_, vi.position_.z + hitRadius_ };
                Irufemi::AABB aabb{ minPos, maxPos };
                
                std::vector<ColliderComponent*> hits;
                cm->QueryAABB(aabb, hits);
                
                if (cm->GetIsDrawDebugLinePtr() && *cm->GetIsDrawDebugLinePtr()) {
                    cm->DrawDebugAABB(aabb, {1.0f, 0.0f, 0.0f, 1.0f});
                }
                
                bool isHit = false;
                for (auto col : hits) {
                    if (!col) continue;
                    auto obj = col->GetGameObject();
                    if (obj && obj->GetName() == "Player") {
                        if (auto playerComp = obj->GetComponent<GravityPlayerComponent>()) {
                            if (!playerComp->IsInvincible()) {
                                Log::OutPutLog(std::cout, "[BossBulletManager] Hit Player!\n");
                                playerComp->TakeDamage(1);
                                isHit = true;
                                break;
                            } else {
                                Log::OutPutLog(std::cout, "[BossBulletManager] Player is invincible.\n");
                            }
                        }
                    }
                }
                
                if (isHit) {
                    ReleaseBullet(vid);
                    continue;
                }
            }
            
            activeVirtualIds_.push(vid); // Keep active
        } else {
            // Already destroyed somewhere else
        }
    }
}

void BossBulletManagerComponent::OnRegisterProperties() {
    RegisterProperty("Max Bullets", &maxBullets_);
    RegisterProperty("Default Life Time", &defaultLifeTime_);
    RegisterPropertyRange("Hit Radius", &hitRadius_, 0.1f, 10.0f);
}

void BossBulletManagerComponent::SpawnBullet(const Irufemi::Vector3& position, const Irufemi::Vector3& velocity) {
    if (!virtualManager_) return;
    
    int vid = virtualManager_->AddVirtualInstance(position, {0,0,0}, bulletScale_);
    if (vid >= 0 && vid < maxBullets_) {
        BossBulletData data;
        data.velocity = velocity;
        data.lifeTimer = defaultLifeTime_;
        bulletDataList_[vid] = data;
        activeVirtualIds_.push(vid);
    }
}

void BossBulletManagerComponent::ReleaseBullet(int virtualId) {
    if (virtualManager_) {
        virtualManager_->RemoveVirtualInstance(virtualId);
    }
}
