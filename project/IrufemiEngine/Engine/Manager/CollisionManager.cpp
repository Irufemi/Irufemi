#include "CollisionManager.h"
#include "Framework/Component/Collider/ColliderComponent.h"
#include "Framework/Component/Collider/AABBColliderComponent.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Renderer/LineInstanced/LineClass.h"
#include <algorithm>

void CollisionManager::Initialize() {
    if (!debugLine_) {
        debugLine_ = std::make_unique<Line3DRegion>();
        debugLine_->Initialize();
    }
}

CollisionManager::~CollisionManager() = default;

void CollisionManager::Clear() {
    colliders_.clear();
}

void CollisionManager::RegisterCollider(ColliderComponent* collider) {
    if (!collider) return;
    // 重複登録防止
    auto it = std::find(colliders_.begin(), colliders_.end(), collider);
    if (it == colliders_.end()) {
        colliders_.push_back(collider);
    }
}

void CollisionManager::UnregisterCollider(ColliderComponent* collider) {
    if (!collider) return;
    auto it = std::find(colliders_.begin(), colliders_.end(), collider);
    if (it != colliders_.end()) {
        colliders_.erase(it);
    }
}

void CollisionManager::CheckAllCollisions() {
    if (colliders_.size() < 2) return;

    for (size_t i = 0; i < colliders_.size(); ++i) {
        for (size_t j = i + 1; j < colliders_.size(); ++j) {
            ColliderComponent* colA = colliders_[i];
            ColliderComponent* colB = colliders_[j];

            if (!colA || !colB) continue;

            // 両方がAABBColliderの場合のみ判定（拡張時はここで型判定して分岐）
            if (colA->GetColliderType() == ColliderComponent::ColliderType::AABB &&
                colB->GetColliderType() == ColliderComponent::ColliderType::AABB) {
                
                AABBColliderComponent* aabbA = static_cast<AABBColliderComponent*>(colA);
                AABBColliderComponent* aabbB = static_cast<AABBColliderComponent*>(colB);

                AABB boxA = aabbA->GetWorldAABB();
                AABB boxB = aabbB->GetWorldAABB();

                // 既存のMath::IsCollisionなどを使用して判定
                // ※Collision.h内でグローバルかMath名前空間かに依存
                // 今回は IsCollision() という名前でオーバーロードされていると想定
                if (Collision::IsCollision(boxA, boxB)) {
                    if (colA->onCollisionEnter_) {
                        colA->onCollisionEnter_(colB);
                    }
                    if (colB->onCollisionEnter_) {
                        colB->onCollisionEnter_(colA);
                    }
                }
            }
        }
    }
}

void CollisionManager::DrawDebug() {
    if (!debugLine_) return;
    if (!isDrawDebugLine_) return; // フラグがオフなら描画しない
    
    debugLine_->ClearInstances();
    
    for (ColliderComponent* collider : colliders_) {
        if (!collider) continue;
        
        if (collider->GetColliderType() == ColliderComponent::ColliderType::AABB) {
            AABBColliderComponent* aabbCol = static_cast<AABBColliderComponent*>(collider);
            AABB aabb = aabbCol->GetWorldAABB();
            
            Vector3 p[8] = {
                { aabb.min.x, aabb.min.y, aabb.min.z },
                { aabb.max.x, aabb.min.y, aabb.min.z },
                { aabb.min.x, aabb.max.y, aabb.min.z },
                { aabb.max.x, aabb.max.y, aabb.min.z },
                { aabb.min.x, aabb.min.y, aabb.max.z },
                { aabb.max.x, aabb.min.y, aabb.max.z },
                { aabb.min.x, aabb.max.y, aabb.max.z },
                { aabb.max.x, aabb.max.y, aabb.max.z }
            };

            Vector4 color = { 0.0f, 1.0f, 0.0f, 1.0f }; // 緑色

            // 底面
            debugLine_->AddInstance(p[0], p[1], color);
            debugLine_->AddInstance(p[1], p[3], color);
            debugLine_->AddInstance(p[3], p[2], color);
            debugLine_->AddInstance(p[2], p[0], color);
            // 上面
            debugLine_->AddInstance(p[4], p[5], color);
            debugLine_->AddInstance(p[5], p[7], color);
            debugLine_->AddInstance(p[7], p[6], color);
            debugLine_->AddInstance(p[6], p[4], color);
            // 縦
            debugLine_->AddInstance(p[0], p[4], color);
            debugLine_->AddInstance(p[1], p[5], color);
            debugLine_->AddInstance(p[2], p[6], color);
            debugLine_->AddInstance(p[3], p[7], color);
        }
    }
    
    debugLine_->Update();
    debugLine_->Draw();
}
