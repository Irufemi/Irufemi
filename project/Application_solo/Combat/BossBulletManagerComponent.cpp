#include "Combat/BossBulletManagerComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/VirtualEntity/VirtualEntityManagerComponent.h"
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Framework/Scene/BaseScene.h"
#include "Player/GravityPlayerComponent.h"
#include "Physics/CollisionManager.h"
#include "Framework/Component/Collider/ColliderComponent.h"
#include "Core/Utility/Log.h"
#include "Renderer/System/VoxelParticle/VoxelParticleManager.h"
#include "Effects/EffectManagerComponent.h"
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
        
        // Find index in dense_ array
        int denseIndex = virtualManager_->GetSparseIndex(vid);
        if (denseIndex >= 0) {
            auto& vi = virtualInstances[denseIndex];

            // 共通の爆発エフェクト処理ラムダ
            auto playExplosion = [&](const Irufemi::Vector3& pos) {
                EffectManagerComponent* effectManager = nullptr;
                if (auto go = gameObject_->GetScene()->FindGameObject("EffectManager")) {
                    effectManager = go->GetComponent<EffectManagerComponent>();
                }
                if (effectManager) {
                    effectManager->PlayEffect(hitEffectKey_, pos);
                }
                
                if (auto voxelManager = BaseModel::GetIrufemiEngine()->GetVoxelParticleManager()) {
                    VoxelEmitter p{};
                    p.particleType = 5; // DebrisExplosive
                    p.lifeTime = 1.0f;
                    p.gravity = 5.0f;
                    p.dispersion = 12.0f;
                    p.scale = {0.5f, 0.5f, 0.5f};
                    
                    Irufemi::Vector4 aura = {0.8f, 0.0f, 0.6f, 0.4f}; // Boss Aura
                    Irufemi::Vector4 rockColor = {1.5f, 1.2f, 1.0f, 1.0f};
                    p.startColor = {rockColor.x + aura.x * 2.0f, rockColor.y + aura.y * 2.0f, rockColor.z + aura.z * 2.0f, 1.0f};
                    p.endColor = {0.2f, 0.2f, 0.2f, 1.0f};
                    p.dissolveEdgeColor = aura;

                    voxelManager->PlayExplosion(explosionModelPath_, pos, {0,0,0}, {0,0,0}, {1,1,1}, p, {2,2,2});
                }
            };

            if (data.lifeTimer <= 0.0f) {
                playExplosion(vi.position_);
                ReleaseBullet(vid);
                continue;
            }
            
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
                                playerComp->TakeDamage(1);
                                isHit = true;
                                break;
                            }
                        }
                    }
                }
                
                if (isHit) {
                    playExplosion(vi.position_);
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
    RegisterProperty("Hit Effect Key", &hitEffectKey_);
    RegisterProperty("Explosion Model Path", &explosionModelPath_);
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
