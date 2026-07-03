#include "CollisionManager.h"
#include "Framework/Component/Collider/ColliderComponent.h"
#include "Framework/Component/Collider/AABBColliderComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Renderer/Object/Line/LineClass.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Core/System/ThreadPool.h"


void CollisionManager::Initialize() {
    if (!debugLine_) {
        debugLine_ = std::make_unique<Line3DBatch>();
        debugLine_->Initialize();
    }
    
    layerNames_ = { "Default" };
    LoadLayers(layerConfigFilePath_);
}

CollisionManager::~CollisionManager() = default;

void CollisionManager::Clear() {
    std::unique_lock<std::shared_mutex> lock(collidersMutex_);
    colliders_.clear();
    previousCollisions_.clear();
}

void CollisionManager::RegisterCollider(ColliderComponent* collider) {
    if (!collider) return;
    std::unique_lock<std::shared_mutex> lock(collidersMutex_);
    // 重複登録防止
    auto it = std::find(colliders_.begin(), colliders_.end(), collider);
    if (it == colliders_.end()) {
        colliders_.push_back(collider);
    }
}

void CollisionManager::UnregisterCollider(ColliderComponent* collider) {
    if (!collider) return;
    
    std::unique_lock<std::shared_mutex> lock(collidersMutex_);
    
    auto it = std::find(colliders_.begin(), colliders_.end(), collider);
    if (it != colliders_.end()) {
        colliders_.erase(it);
    }

    // 削除されるコライダーが含まれているペアをpreviousCollisions_から削除し、Exitを呼ぶ
    for (auto iter = previousCollisions_.begin(); iter != previousCollisions_.end(); ) {
        if (iter->first == collider || iter->second == collider) {
            ColliderComponent* other = (iter->first == collider) ? iter->second : iter->first;
            
            // 削除される側からExitを呼ぶ（任意）
            // 既にデストラクタが走っているオブジェクトのメソッドを呼ぶとVTable参照エラー（純粋仮想関数呼び出し等）になるため、
            // 削除対象(collider)が属するGameObjectへのExit通知は行わない。
            // また、other側へExit通知を送る際も、colliderが破棄中であることに注意が必要。
            // 可能であれば、other->GetGameObject()->SendCollisionExit(nullptr) のようにするか、通知自体をスキップする。
            
            if (other && other->onCollisionExit_) {
                other->onCollisionExit_(nullptr); // 破棄されるオブジェクトへのアクセスを防ぐためnullptrを渡す
            }
            if (other && other->GetGameObject()) {
                other->GetGameObject()->SendCollisionExit(nullptr);
            }
            
            iter = previousCollisions_.erase(iter);
        } else {
            ++iter;
        }
    }
}

void CollisionManager::CheckAllCollisions() {
    std::set<std::pair<ColliderComponent*, ColliderComponent*>> currentCollisions;

    if (colliders_.size() >= 2) {
        for (size_t i = 0; i < colliders_.size(); ++i) {
            ColliderComponent* colA = colliders_[i];
            if (!colA || !colA->GetGameObject() || !colA->GetGameObject()->GetIsActive()) continue;

            for (size_t j = i + 1; j < colliders_.size(); ++j) {
                ColliderComponent* colB = colliders_[j];
                if (!colB || !colB->GetGameObject() || !colB->GetGameObject()->GetIsActive()) continue;

                // フィルタリング
                if ((colA->mask_ & colB->layer_) == 0 || (colB->mask_ & colA->layer_) == 0) {
                    continue;
                }

                // アドレスでソートしてペアを作成
                auto pair = colA < colB ? std::make_pair(colA, colB) : std::make_pair(colB, colA);

                // --- 判定ディスパッチ ---
                Collision::CollisionResult result;

                if (colA->GetColliderType() == ColliderComponent::ColliderType::AABB) {
                    AABB boxA = static_cast<AABBColliderComponent*>(colA)->GetWorldAABB();
                    
                    if (colB->GetColliderType() == ColliderComponent::ColliderType::AABB) {
                        AABB boxB = static_cast<AABBColliderComponent*>(colB)->GetWorldAABB();
                        result = Collision::GetCollisionResult(boxA, boxB);
                    } 
                    else if (colB->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
                        Sphere sphereB = static_cast<SphereColliderComponent*>(colB)->GetWorldSphere();
                        result = Collision::GetCollisionResult(boxA, sphereB);
                    }
                    else if (colB->GetColliderType() == ColliderComponent::ColliderType::OBB) {
                        OBB obbB = static_cast<OBBColliderComponent*>(colB)->GetWorldOBB();
                        result = Collision::GetCollisionResult(obbB, boxA); // OBB vs AABB
                        result.normal = Math::Multiply(-1.0f, result.normal); // OBBを押し出す方向の逆にする
                    }
                }
                else if (colA->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
                    Sphere sphereA = static_cast<SphereColliderComponent*>(colA)->GetWorldSphere();
                    
                    if (colB->GetColliderType() == ColliderComponent::ColliderType::AABB) {
                        AABB boxB = static_cast<AABBColliderComponent*>(colB)->GetWorldAABB();
                        result = Collision::GetCollisionResult(boxB, sphereA);
                        result.normal = Math::Multiply(-1.0f, result.normal);
                    }
                    else if (colB->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
                        Sphere sphereB = static_cast<SphereColliderComponent*>(colB)->GetWorldSphere();
                        result = Collision::GetCollisionResult(sphereA, sphereB);
                    }
                    else if (colB->GetColliderType() == ColliderComponent::ColliderType::OBB) {
                        OBB obbB = static_cast<OBBColliderComponent*>(colB)->GetWorldOBB();
                        result = Collision::GetCollisionResult(obbB, sphereA);
                        result.normal = Math::Multiply(-1.0f, result.normal);
                    }
                }
                else if (colA->GetColliderType() == ColliderComponent::ColliderType::OBB) {
                    OBB obbA = static_cast<OBBColliderComponent*>(colA)->GetWorldOBB();
                    
                    if (colB->GetColliderType() == ColliderComponent::ColliderType::AABB) {
                        AABB boxB = static_cast<AABBColliderComponent*>(colB)->GetWorldAABB();
                        result = Collision::GetCollisionResult(obbA, boxB);
                    }
                    else if (colB->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
                        Sphere sphereB = static_cast<SphereColliderComponent*>(colB)->GetWorldSphere();
                        result = Collision::GetCollisionResult(obbA, sphereB);
                    }
                    else if (colB->GetColliderType() == ColliderComponent::ColliderType::OBB) {
                        OBB obbB = static_cast<OBBColliderComponent*>(colB)->GetWorldOBB();
                        result = Collision::GetCollisionResult(obbA, obbB);
                    }
                }

                if (result.isHit) {
                    currentCollisions.insert(pair);

                    // --- コールバック呼び出し (Enter / Stay) ---
                    if (previousCollisions_.find(pair) == previousCollisions_.end()) {
                        // 新規衝突 (Enter)
                        if (colA->onCollisionEnter_) colA->onCollisionEnter_(colB);
                        if (colB->onCollisionEnter_) colB->onCollisionEnter_(colA);
                        
                        if (colA->GetGameObject()) colA->GetGameObject()->SendCollisionEnter(colB->GetGameObject());
                        if (colB->GetGameObject()) colB->GetGameObject()->SendCollisionEnter(colA->GetGameObject());
                    } else {
                        // 継続衝突 (Stay)
                        if (colA->onCollisionStay_) colA->onCollisionStay_(colB);
                        if (colB->onCollisionStay_) colB->onCollisionStay_(colA);
                        
                        if (colA->GetGameObject()) colA->GetGameObject()->SendCollisionStay(colB->GetGameObject());
                        if (colB->GetGameObject()) colB->GetGameObject()->SendCollisionStay(colA->GetGameObject());
                    }

                    // --- 押し戻し処理 (Kinematic Resolution) ---
                    if (!colA->isTrigger_ && !colB->isTrigger_) {
                        TransformComponent* transformA = colA->GetGameObject() ? colA->GetGameObject()->GetComponent<TransformComponent>() : nullptr;
                        TransformComponent* transformB = colB->GetGameObject() ? colB->GetGameObject()->GetComponent<TransformComponent>() : nullptr;

                        if (transformA && transformB) {
                            // 両方動く場合は半分の距離ずつ押し戻す
                            Vector3 pushA = Math::Multiply(result.depth * 0.5f, result.normal);
                            Vector3 pushB = Math::Multiply(result.depth * 0.5f, Math::Multiply(-1.0f, result.normal));
                            
                            transformA->SetPosition(Math::Add(transformA->GetPosition(), pushA));
                            transformB->SetPosition(Math::Add(transformB->GetPosition(), pushB));
                        } else if (transformA) {
                            Vector3 pushA = Math::Multiply(result.depth, result.normal);
                            transformA->SetPosition(Math::Add(transformA->GetPosition(), pushA));
                        } else if (transformB) {
                            Vector3 pushB = Math::Multiply(result.depth, Math::Multiply(-1.0f, result.normal));
                            transformB->SetPosition(Math::Add(transformB->GetPosition(), pushB));
                        }
                    }
                }
            }
        }
    }

    // --- 離脱処理 (Exit) ---
    for (const auto& pair : previousCollisions_) {
        // 前フレームでは当たっていたが、今フレームでは当たっていない
        if (currentCollisions.find(pair) == currentCollisions.end()) {
            ColliderComponent* colA = pair.first;
            ColliderComponent* colB = pair.second;
            
            if (colA && colA->onCollisionExit_) colA->onCollisionExit_(colB);
            if (colB && colB->onCollisionExit_) colB->onCollisionExit_(colA);
            
            if (colA && colA->GetGameObject()) colA->GetGameObject()->SendCollisionExit(colB ? colB->GetGameObject() : nullptr);
            if (colB && colB->GetGameObject()) colB->GetGameObject()->SendCollisionExit(colA ? colA->GetGameObject() : nullptr);
        }
    }

    // 更新
    previousCollisions_ = std::move(currentCollisions);
}

void CollisionManager::DrawDebug(GameObject* selectedObject) {
    if (!debugLine_) return;
    
    debugLine_->ClearInstances();
    
    for (ColliderComponent* collider : colliders_) {
        if (!collider || !collider->GetGameObject() || !collider->GetGameObject()->GetIsActive()) continue;
        
        bool isSelected = false;
        if (selectedObject) {
            GameObject* colObj = collider->GetGameObject();
            GameObject* selObj = selectedObject;

            // コライダーの持ち主が、選択されたオブジェクトの親（または同一）か？
            while (selObj) {
                if (colObj == selObj) {
                    isSelected = true;
                    break;
                }
                selObj = selObj->GetParent().get();
            }
            
            // 逆に、コライダーの持ち主が、選択されたオブジェクトの子孫か？
            if (!isSelected) {
                while (colObj) {
                    if (colObj == selectedObject) {
                        isSelected = true;
                        break;
                    }
                    colObj = colObj->GetParent().get();
                }
            }
        }
        
        // 全体表示OFFのときでも、選択中のオブジェクト（またはその親・子）のコライダーは表示する
        if (!isDrawDebugLine_ && !isSelected) continue;

        Vector4 color = isSelected ? Vector4{ 1.0f, 0.5f, 0.0f, 1.0f } : Vector4{ 0.0f, 1.0f, 0.0f, 1.0f };

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

            // AABB
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
        else if (collider->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
            SphereColliderComponent* sphereCol = static_cast<SphereColliderComponent*>(collider);
            Sphere sphere = sphereCol->GetWorldSphere();
            
            // 簡単な3軸の円弧近似を描画
            int segments = 32;
            for (int i = 0; i < segments; ++i) {
                float theta1 = (static_cast<float>(i) / segments) * 2.0f * 3.14159265f;
                float theta2 = (static_cast<float>(i + 1) / segments) * 2.0f * 3.14159265f;
                
                // X-Y plane
                Vector3 p1_xy = sphere.center + Vector3{ std::cos(theta1), std::sin(theta1), 0.0f } * sphere.radius;
                Vector3 p2_xy = sphere.center + Vector3{ std::cos(theta2), std::sin(theta2), 0.0f } * sphere.radius;
                debugLine_->AddInstance(p1_xy, p2_xy, color);
                
                // Y-Z plane
                Vector3 p1_yz = sphere.center + Vector3{ 0.0f, std::cos(theta1), std::sin(theta1) } * sphere.radius;
                Vector3 p2_yz = sphere.center + Vector3{ 0.0f, std::cos(theta2), std::sin(theta2) } * sphere.radius;
                debugLine_->AddInstance(p1_yz, p2_yz, color);
                
                // Z-X plane
                Vector3 p1_zx = sphere.center + Vector3{ std::sin(theta1), 0.0f, std::cos(theta1) } * sphere.radius;
                Vector3 p2_zx = sphere.center + Vector3{ std::sin(theta2), 0.0f, std::cos(theta2) } * sphere.radius;
                debugLine_->AddInstance(p1_zx, p2_zx, color);
            }
        }
        else if (collider->GetColliderType() == ColliderComponent::ColliderType::OBB) {
            OBBColliderComponent* obbCol = static_cast<OBBColliderComponent*>(collider);
            OBB obb = obbCol->GetWorldOBB();
            
            // 8頂点を計算
            Vector3 axes[3] = { obb.orientations[0], obb.orientations[1], obb.orientations[2] };
            Vector3 extents = obb.size;
            Vector3 center = obb.center;
            
            Vector3 dx = axes[0] * extents.x;
            Vector3 dy = axes[1] * extents.y;
            Vector3 dz = axes[2] * extents.z;
            
            Vector3 p[8] = {
                center - dx - dy - dz,
                center + dx - dy - dz,
                center - dx + dy - dz,
                center + dx + dy - dz,
                center - dx - dy + dz,
                center + dx - dy + dz,
                center - dx + dy + dz,
                center + dx + dy + dz
            };
            
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
    } // end for colliders_
    
    // Raycastのデバッグ描画（コライダーの描画フラグとは独立して描画）
    for (const auto& r : debugRays_) {
        Vector3 dir = Math::Normalize(r.ray.diff);
        float drawDist = r.distance > 1000.0f ? 1000.0f : r.distance;
        Vector3 endPoint = r.ray.origin + dir * drawDist;
        debugLine_->AddInstance(r.ray.origin, endPoint, r.color);
    }
    debugRays_.clear();

    debugLine_->Update();
    debugLine_->Draw();
}

void CollisionManager::LoadLayers(const std::string& filepath) {
    std::ifstream file(filepath);
    if (file.is_open()) {
        try {
            nlohmann::json j;
            file >> j;
            if (j.contains("layers") && j["layers"].is_array()) {
                layerNames_.clear();
                for (const auto& name : j["layers"]) {
                    layerNames_.push_back(name);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to load layers config: " << e.what() << "\n";
        }
    }
}

void CollisionManager::SaveLayers(const std::string& filepath) {
    nlohmann::json j;
    j["layers"] = layerNames_;
    
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << j.dump(4);
    }
}

void CollisionManager::AddLayer(const std::string& name) {
    if (layerNames_.size() < 32) {
        layerNames_.push_back(name);
        SaveLayers(layerConfigFilePath_);
    }
}

void CollisionManager::RemoveLayer(int index) {
    if (index > 0 && index < layerNames_.size()) { // Default(index=0)は消せないようにする
        layerNames_.erase(layerNames_.begin() + index);
        SaveLayers(layerConfigFilePath_);
    }
}

void CollisionManager::RenameLayer(int index, const std::string& name) {
    if (index > 0 && index < layerNames_.size()) {
        layerNames_[index] = name;
        SaveLayers(layerConfigFilePath_);
    }
}



bool CollisionManager::Raycast(const Ray& ray, RaycastHit& hitInfo, float maxDistance, uint32_t layerMask, GameObject* ignoreObject) {
    hitInfo.isHit = false;
    hitInfo.distance = maxDistance;

    std::shared_lock<std::shared_mutex> lock(collidersMutex_);

    for (ColliderComponent* collider : colliders_) {
        if (!collider || !collider->GetGameObject() || !collider->GetGameObject()->GetIsActive()) continue;

        // 除外オブジェクトならスキップ
        if (ignoreObject && collider->GetGameObject() == ignoreObject) continue;

        // 指定されたレイヤーマスクに合致するか判定
        if ((collider->layer_ & layerMask) == 0) continue;

        float distance = 0.0f;
        bool isHit = false;

        switch (collider->GetColliderType()) {
        case ColliderComponent::ColliderType::AABB: {
            AABBColliderComponent* aabbCol = static_cast<AABBColliderComponent*>(collider);
            isHit = Collision::IsCollision(ray, aabbCol->GetWorldAABB(), distance);
            break;
        }
        case ColliderComponent::ColliderType::Sphere: {
            SphereColliderComponent* sphereCol = static_cast<SphereColliderComponent*>(collider);
            isHit = Collision::IsCollision(ray, sphereCol->GetWorldSphere(), distance);
            break;
        }
        case ColliderComponent::ColliderType::OBB: {
            OBBColliderComponent* obbCol = static_cast<OBBColliderComponent*>(collider);
            isHit = Collision::IsCollision(ray, obbCol->GetWorldOBB(), distance);
            break;
        }
        }

        if (isHit && distance < hitInfo.distance) {
            hitInfo.isHit = true;
            hitInfo.distance = distance;
            hitInfo.hitCollider = collider;
            hitInfo.hitObject = collider->GetGameObject();
            hitInfo.hitPoint = ray.origin + Math::Normalize(ray.diff) * distance;
        }
    }

    return hitInfo.isHit;
}

void CollisionManager::DrawDebugRay(const Ray& ray, float distance, const Vector4& color) {
    debugRays_.push_back({ ray, distance, color });
}

std::future<std::pair<bool, RaycastHit>> CollisionManager::RaycastAsync(ThreadPool* pool, const Ray& ray, float maxDistance, uint32_t layerMask, GameObject* ignoreObject) {
    if (!pool) {
        std::promise<std::pair<bool, RaycastHit>> prom;
        RaycastHit hit;
        bool result = Raycast(ray, hit, maxDistance, layerMask, ignoreObject);
        prom.set_value({result, hit});
        return prom.get_future();
    }
    
    return pool->Enqueue([this, ray, maxDistance, layerMask, ignoreObject]() -> std::pair<bool, RaycastHit> {
        RaycastHit hit;
        bool result = this->Raycast(ray, hit, maxDistance, layerMask, ignoreObject);
        return {result, hit};
    });
}
