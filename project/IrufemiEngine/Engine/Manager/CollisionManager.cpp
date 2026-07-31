#include "CollisionManager.h"
#include "Engine/Core/Utility/Log.h"
#include <iostream>
#include "Framework/Component/Collider/ColliderComponent.h"
#include "Framework/Component/Collider/AABBColliderComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Renderer/Object/Line/LineClass.h"
#include "Renderer/Object/Batch/DebugPrimitiveRenderer.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Core/System/ThreadPool.h"


void CollisionManager::Initialize(DebugPrimitiveRenderer* debugRenderer) {
    debugPrimitiveRenderer_ = debugRenderer;
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
    dynamicBVH_.Clear();
}

void CollisionManager::RegisterCollider(ColliderComponent* collider) {
    if (!collider) return;
    std::unique_lock<std::shared_mutex> lock(collidersMutex_);
    // 重複登録防止
    auto it = std::find(colliders_.begin(), colliders_.end(), collider);
    if (it == colliders_.end()) {
        colliders_.push_back(collider);
        collider->bvhNodeId_ = dynamicBVH_.Insert(collider, collider->GetBoundingBox());
    }
}

void CollisionManager::UnregisterCollider(ColliderComponent* collider) {
    if (!collider) return;
    
    std::unique_lock<std::shared_mutex> lock(collidersMutex_);
    
    auto it = std::find(colliders_.begin(), colliders_.end(), collider);
    if (it != colliders_.end()) {
        colliders_.erase(it);
        dynamicBVH_.Remove(collider->bvhNodeId_);
        collider->bvhNodeId_ = -1;
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

    // --- BVH Update Phase ---
    for (ColliderComponent* collider : colliders_) {
        if (!collider || !collider->GetGameObject() || !collider->GetGameObject()->GetIsActive()) continue;
        dynamicBVH_.Update(collider->bvhNodeId_, collider->GetBoundingBox());
    }

    // --- Broad Phase ---
    std::vector<ColliderComponent*> potentialHits;

    for (size_t i = 0; i < colliders_.size(); ++i) {
        ColliderComponent* colA = colliders_[i];
        if (!colA || !colA->GetGameObject() || !colA->GetGameObject()->GetIsActive()) continue;

        potentialHits.clear();
        dynamicBVH_.Query(colA->GetBoundingBox(), potentialHits);

        for (ColliderComponent* colB : potentialHits) {
            if (!colB || colA == colB) continue;
            if (!colB->GetGameObject() || !colB->GetGameObject()->GetIsActive()) continue;

            // 重複判定を防ぐため、アドレスが小さい方から大きい方へのみ判定を行う
            if (colA >= colB) continue;

            // フィルタリング
            if ((colA->mask_ & colB->layer_) == 0 || (colB->mask_ & colA->layer_) == 0) {
                continue;
            }

            // アドレスでソートしてペアを作成 (colA < colB is guaranteed)
            auto pairKey = std::make_pair(colA, colB);

            // --- Narrow Phase (判定ディスパッチ) ---
            Irufemi::Collision::CollisionResult result;

                if (colA->GetColliderType() == ColliderComponent::ColliderType::AABB) {
                    Irufemi::AABB boxA = static_cast<AABBColliderComponent*>(colA)->GetWorldAABB();
                    
                    if (colB->GetColliderType() == ColliderComponent::ColliderType::AABB) {
                        Irufemi::AABB boxB = static_cast<AABBColliderComponent*>(colB)->GetWorldAABB();
                        result = Irufemi::Collision::GetCollisionResult(boxA, boxB);
                    } 
                    else if (colB->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
                        Irufemi::Sphere sphereB = static_cast<SphereColliderComponent*>(colB)->GetWorldSphere();
                        result = Irufemi::Collision::GetCollisionResult(boxA, sphereB);
                    }
                    else if (colB->GetColliderType() == ColliderComponent::ColliderType::OBB) {
                        Irufemi::OBB obbB = static_cast<OBBColliderComponent*>(colB)->GetWorldOBB();
                        result = Irufemi::Collision::GetCollisionResult(obbB, boxA); // Irufemi::OBB vs Irufemi::AABB
                        result.normal = Irufemi::Math::Multiply(-1.0f, result.normal); // OBBを押し出す方向の逆にする
                    }
                }
                else if (colA->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
                    Irufemi::Sphere sphereA = static_cast<SphereColliderComponent*>(colA)->GetWorldSphere();
                    
                    if (colB->GetColliderType() == ColliderComponent::ColliderType::AABB) {
                        Irufemi::AABB boxB = static_cast<AABBColliderComponent*>(colB)->GetWorldAABB();
                        result = Irufemi::Collision::GetCollisionResult(boxB, sphereA);
                        result.normal = Irufemi::Math::Multiply(-1.0f, result.normal);
                    }
                    else if (colB->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
                        Irufemi::Sphere sphereB = static_cast<SphereColliderComponent*>(colB)->GetWorldSphere();
                        result = Irufemi::Collision::GetCollisionResult(sphereA, sphereB);
                    }
                    else if (colB->GetColliderType() == ColliderComponent::ColliderType::OBB) {
                        Irufemi::OBB obbB = static_cast<OBBColliderComponent*>(colB)->GetWorldOBB();
                        result = Irufemi::Collision::GetCollisionResult(obbB, sphereA);
                        result.normal = Irufemi::Math::Multiply(-1.0f, result.normal);
                    }
                }
                else if (colA->GetColliderType() == ColliderComponent::ColliderType::OBB) {
                    Irufemi::OBB obbA = static_cast<OBBColliderComponent*>(colA)->GetWorldOBB();
                    
                    if (colB->GetColliderType() == ColliderComponent::ColliderType::AABB) {
                        Irufemi::AABB boxB = static_cast<AABBColliderComponent*>(colB)->GetWorldAABB();
                        result = Irufemi::Collision::GetCollisionResult(obbA, boxB);
                    }
                    else if (colB->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
                        Irufemi::Sphere sphereB = static_cast<SphereColliderComponent*>(colB)->GetWorldSphere();
                        result = Irufemi::Collision::GetCollisionResult(obbA, sphereB);
                    }
                    else if (colB->GetColliderType() == ColliderComponent::ColliderType::OBB) {
                        Irufemi::OBB obbB = static_cast<OBBColliderComponent*>(colB)->GetWorldOBB();
                        result = Irufemi::Collision::GetCollisionResult(obbA, obbB);
                    }
                }

                if (result.isHit) {
                    currentCollisions.insert(pairKey);

                    // --- コールバック呼び出し (Enter / Stay) ---
                    if (previousCollisions_.find(pairKey) == previousCollisions_.end()) {
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
                            Irufemi::Vector3 pushA = Irufemi::Math::Multiply(result.depth * 0.5f, result.normal);
                            Irufemi::Vector3 pushB = Irufemi::Math::Multiply(result.depth * 0.5f, Irufemi::Math::Multiply(-1.0f, result.normal));
                            
                            transformA->SetWorldPosition(Irufemi::Math::Add(transformA->GetWorldPosition(), pushA));
                            transformB->SetWorldPosition(Irufemi::Math::Add(transformB->GetWorldPosition(), pushB));
                        } else if (transformA) {
                            Irufemi::Vector3 pushA = Irufemi::Math::Multiply(result.depth, result.normal);
                            transformA->SetWorldPosition(Irufemi::Math::Add(transformA->GetWorldPosition(), pushA));
                        } else if (transformB) {
                            Irufemi::Vector3 pushB = Irufemi::Math::Multiply(result.depth, Irufemi::Math::Multiply(-1.0f, result.normal));
                            transformB->SetWorldPosition(Irufemi::Math::Add(transformB->GetWorldPosition(), pushB));
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

        Irufemi::Vector4 color = isSelected ? Irufemi::Vector4{ 1.0f, 0.5f, 0.0f, 1.0f } : Irufemi::Vector4{ 0.0f, 1.0f, 0.0f, 1.0f };

        if (collider->GetColliderType() == ColliderComponent::ColliderType::AABB) {
            AABBColliderComponent* aabbCol = static_cast<AABBColliderComponent*>(collider);
            Irufemi::AABB aabb = aabbCol->GetWorldAABB();
            
            Irufemi::Vector3 size = { aabb.max.x - aabb.min.x, aabb.max.y - aabb.min.y, aabb.max.z - aabb.min.z };
            Irufemi::Vector3 center = { (aabb.max.x + aabb.min.x) * 0.5f, (aabb.max.y + aabb.min.y) * 0.5f, (aabb.max.z + aabb.min.z) * 0.5f };
            Irufemi::Matrix4x4 transform = Irufemi::Math::MakeAffineMatrix(size, Irufemi::Vector3{0, 0, 0}, center);
            
            if (debugPrimitiveRenderer_) {
                debugPrimitiveRenderer_->AddCube(transform, color);
            }
        }
        else if (collider->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
            SphereColliderComponent* sphereCol = static_cast<SphereColliderComponent*>(collider);
            Irufemi::Sphere sphere = sphereCol->GetWorldSphere();
            
            if (debugPrimitiveRenderer_) {
                debugPrimitiveRenderer_->AddSphere(sphere.center, sphere.radius, color);
            }
        }
        else if (collider->GetColliderType() == ColliderComponent::ColliderType::OBB) {
            OBBColliderComponent* obbCol = static_cast<OBBColliderComponent*>(collider);
            Irufemi::OBB obb = obbCol->GetWorldOBB();
            
            Irufemi::Matrix4x4 transform;
            transform.m[0][0] = obb.orientations[0].x * obb.size.x;
            transform.m[0][1] = obb.orientations[0].y * obb.size.x;
            transform.m[0][2] = obb.orientations[0].z * obb.size.x;
            transform.m[0][3] = 0.0f;
            transform.m[1][0] = obb.orientations[1].x * obb.size.y;
            transform.m[1][1] = obb.orientations[1].y * obb.size.y;
            transform.m[1][2] = obb.orientations[1].z * obb.size.y;
            transform.m[1][3] = 0.0f;
            transform.m[2][0] = obb.orientations[2].x * obb.size.z;
            transform.m[2][1] = obb.orientations[2].y * obb.size.z;
            transform.m[2][2] = obb.orientations[2].z * obb.size.z;
            transform.m[2][3] = 0.0f;
            transform.m[3][0] = obb.center.x;
            transform.m[3][1] = obb.center.y;
            transform.m[3][2] = obb.center.z;
            transform.m[3][3] = 1.0f;
            
            if (debugPrimitiveRenderer_) {
                debugPrimitiveRenderer_->AddCube(transform, color);
            }
        }
    } // end for colliders_
    
    // Raycastのデバッグ描画（コライダーの描画フラグとは独立して描画）
    for (const auto& r : debugRays_) {
        Irufemi::Vector3 dir = Irufemi::Math::Normalize(r.ray.diff);
        float drawDist = r.distance > 1000.0f ? 1000.0f : r.distance;
        Irufemi::Vector3 endPoint = r.ray.origin + dir * drawDist;
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
            /**
             * @brief エディタのコンソールパネルにも出力するため、Log::OutPutLog を使用
             */
            Log::OutPutLog(std::cerr, "Failed to load layers config: " + std::string(e.what()));
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

uint32_t CollisionManager::GetLayerMask(const std::string& name) const {
    for (size_t i = 0; i < layerNames_.size(); ++i) {
        if (layerNames_[i] == name) {
            return 1 << i;
        }
    }
    return 0; // Not found
}

bool CollisionManager::Raycast(const Irufemi::Ray& ray, RaycastHit& hitInfo, float maxDistance, uint32_t layerMask, GameObject* ignoreObject) {
    hitInfo.isHit = false;
    hitInfo.distance = maxDistance;

    std::shared_lock<std::shared_mutex> lock(collidersMutex_);

    std::vector<ColliderComponent*> potentialHits;
    dynamicBVH_.RaycastQuery(ray, maxDistance, potentialHits);

    for (ColliderComponent* collider : potentialHits) {
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
            isHit = Irufemi::Collision::IsCollision(ray, aabbCol->GetWorldAABB(), distance);
            break;
        }
        case ColliderComponent::ColliderType::Sphere: {
            SphereColliderComponent* sphereCol = static_cast<SphereColliderComponent*>(collider);
            isHit = Irufemi::Collision::IsCollision(ray, sphereCol->GetWorldSphere(), distance);
            break;
        }
        case ColliderComponent::ColliderType::OBB: {
            OBBColliderComponent* obbCol = static_cast<OBBColliderComponent*>(collider);
            isHit = Irufemi::Collision::IsCollision(ray, obbCol->GetWorldOBB(), distance);
            break;
        }
        }

        if (isHit && distance < hitInfo.distance) {
            hitInfo.isHit = true;
            hitInfo.distance = distance;
            hitInfo.hitCollider = collider;
            hitInfo.hitObject = collider->GetGameObject();
            hitInfo.hitPoint = ray.origin + Irufemi::Math::Normalize(ray.diff) * distance;
        }
    }

    return hitInfo.isHit;
}

void CollisionManager::DrawDebugRay(const Irufemi::Ray& ray, float distance, const Irufemi::Vector4& color) {
    debugRays_.push_back({ ray, distance, color });
}

std::future<std::pair<bool, RaycastHit>> CollisionManager::RaycastAsync(ThreadPool* pool, const Irufemi::Ray& ray, float maxDistance, uint32_t layerMask, GameObject* ignoreObject) {
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
