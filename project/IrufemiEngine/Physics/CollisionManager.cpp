#include "Physics/CollisionManager.h"
#include "Core/Utility/Log.h"
#include <iostream>
#include "Framework/Component/Collider/ColliderComponent.h"
#include "Framework/Component/Collider/AABBColliderComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Physics/Collision/Collision.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Renderer/Object/Line/LineClass.h"
#include "Renderer/Object/Batch/DebugPrimitiveRenderer.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include "Core/Math/MathFunction.h"
#include "Core/System/ThreadPool.h"

void CollisionManager::Initialize(DebugPrimitiveRenderer* debugRenderer) {
    debugPrimitiveRenderer_ = debugRenderer;
    if (!debugLine_) {
        debugLine_ = std::make_unique<Line3DBatch>();
        debugLine_->Initialize();
    }

    layerNames_ = {"Default"};
    LoadLayers(layerConfigFilePath_);
}

CollisionManager::~CollisionManager() = default;

void CollisionManager::Clear() {
    std::unique_lock<std::shared_mutex> lock(collidersMutex_);
    colliders_.clear();
    previousCollisions_.clear();
    dynamicBVH_.Clear();

    std::lock_guard<std::mutex> pendingLock(pendingMutex_);
    pendingAdds_.clear();
    pendingRemoves_.clear();
}

void CollisionManager::RegisterCollider(ColliderComponent* collider) {
    if (!collider) {
        return;
    }
    std::lock_guard<std::mutex> lock(pendingMutex_);
    // 重複を避ける
    if (std::find(pendingAdds_.begin(), pendingAdds_.end(), collider) == pendingAdds_.end()) {
        pendingAdds_.push_back(collider);
    }
}

void CollisionManager::UnregisterCollider(ColliderComponent* collider) {
    if (!collider) {
        return;
    }
    std::lock_guard<std::mutex> lock(pendingMutex_);
    if (std::find(pendingRemoves_.begin(), pendingRemoves_.end(), collider) == pendingRemoves_.end()) {
        pendingRemoves_.push_back(collider);
    }
}

void CollisionManager::FlushPendingCommands() {
    std::vector<ColliderComponent*> adds;
    std::vector<ColliderComponent*> removes;

    {
        std::lock_guard<std::mutex> pendingLock(pendingMutex_);
        adds = std::move(pendingAdds_);
        removes = std::move(pendingRemoves_);
    }

    if (adds.empty() && removes.empty()) {
        return;
    }

    std::unique_lock<std::shared_mutex> lock(collidersMutex_);

    // 削除の適用
    for (ColliderComponent* collider : removes) {
        auto it = std::find(colliders_.begin(), colliders_.end(), collider);
        if (it != colliders_.end()) {
            colliders_.erase(it);
            if (collider->bvhNodeId_ != -1) {
                dynamicBVH_.Remove(collider->bvhNodeId_);
                collider->bvhNodeId_ = -1;
            }
        }

        for (auto iter = previousCollisions_.begin(); iter != previousCollisions_.end();) {
            if (iter->first == collider || iter->second == collider) {
                ColliderComponent* other = (iter->first == collider) ? iter->second : iter->first;
                if (other && other->onCollisionExit_) {
                    other->onCollisionExit_(nullptr);
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

    // 追加の適用
    for (ColliderComponent* collider : adds) {
        // まだ削除されていないか（削除キューに入っていなかったか）と重複を確認
        if (std::find(removes.begin(), removes.end(), collider) == removes.end()) {
            auto it = std::find(colliders_.begin(), colliders_.end(), collider);
            if (it == colliders_.end()) {
                colliders_.push_back(collider);
                collider->bvhNodeId_ = dynamicBVH_.Insert(collider, collider->GetBoundingBox());
            }
        }
    }
}

void CollisionManager::CheckAllCollisions() {
    FlushPendingCommands();

    std::set<std::pair<ColliderComponent*, ColliderComponent*>> currentCollisions;

    // --- BVH Update Phase ---
    {
        // BVHの更新はツリー構造の変更を伴うため、排他ロック（Unique Lock）を取得する
        std::unique_lock<std::shared_mutex> lock(collidersMutex_);
        for (ColliderComponent* collider : colliders_) {
            if (!collider) {
                continue;
            }
            auto go = collider->GetGameObject();
            if (!go) {
                continue;
            }
            if (reinterpret_cast<uintptr_t>(go) < 0x1000) {
                Log::OutPutLog(std::cerr, "[CollisionManager] CRITICAL ERROR: Caught invalid GameObject pointer (0x" +
                                              std::format("{:X}", reinterpret_cast<uintptr_t>(go)) +
                                              ") in CollisionManager!\n");
                continue;
            }
            if (!go->GetIsActive()) {
                continue;
            }
            dynamicBVH_.Update(collider->bvhNodeId_, collider->GetBoundingBox());
        }
    }

    // --- Broad Phase ---
    // BVHの更新が終わったため、判定自体はリードロック（Shared Lock）で行う
    std::shared_lock<std::shared_mutex> sharedLock(collidersMutex_);
    std::vector<ColliderComponent*> potentialHits;

    for (size_t i = 0; i < colliders_.size(); ++i) {
        ColliderComponent* colA = colliders_[i];
        if (!colA) {
            continue;
        }
        auto goA = colA->GetGameObject();
        if (!goA) {
            continue;
        }
        if (reinterpret_cast<uintptr_t>(goA) < 0x1000) {
            Log::OutPutLog(std::cerr, "[CollisionManager] CRITICAL ERROR: Caught invalid GameObject pointer (0x" +
                                          std::format("{:X}", reinterpret_cast<uintptr_t>(goA)) +
                                          ") in CheckAllCollisions (colA)!\n");
            continue;
        }
        if (!goA->GetIsActive()) {
            continue;
        }

        potentialHits.clear();
        dynamicBVH_.Query(colA->GetBoundingBox(), potentialHits);

        for (ColliderComponent* colB : potentialHits) {
            if (!colB || colA == colB) {
                continue;
            }
            auto goB = colB->GetGameObject();
            if (!goB) {
                continue;
            }
            if (reinterpret_cast<uintptr_t>(goB) < 0x1000) {
                Log::OutPutLog(std::cerr, "[CollisionManager] CRITICAL ERROR: Caught invalid GameObject pointer (0x" +
                                              std::format("{:X}", reinterpret_cast<uintptr_t>(goB)) +
                                              ") in CheckAllCollisions (colB)!\n");
                continue;
            }
            if (!goB->GetIsActive()) {
                continue;
            }

            // 重複判定を防ぐため、アドレスが小さい方から大きい方へのみ判定を行う
            if (colA >= colB) {
                continue;
            }

            // フィルタリング
            if ((colA->mask_ & colB->layer_) == 0 || (colB->mask_ & colA->layer_) == 0) {
                continue;
            }

            // 静的オブジェクト同士の判定は不要（動かないため、計算負荷を削減）
            if (colA->isStatic_ && colB->isStatic_) {
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
                } else if (colB->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
                    Irufemi::Sphere sphereB = static_cast<SphereColliderComponent*>(colB)->GetWorldSphere();
                    result = Irufemi::Collision::GetCollisionResult(boxA, sphereB);
                } else if (colB->GetColliderType() == ColliderComponent::ColliderType::OBB) {
                    Irufemi::OBB obbB = static_cast<OBBColliderComponent*>(colB)->GetWorldOBB();
                    result = Irufemi::Collision::GetCollisionResult(obbB, boxA); // Irufemi::OBB vs Irufemi::AABB
                    result.normal = Irufemi::Math::Multiply(-1.0f, result.normal); // OBBを押し出す方向の逆にする
                }
            } else if (colA->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
                Irufemi::Sphere sphereA = static_cast<SphereColliderComponent*>(colA)->GetWorldSphere();

                if (colB->GetColliderType() == ColliderComponent::ColliderType::AABB) {
                    Irufemi::AABB boxB = static_cast<AABBColliderComponent*>(colB)->GetWorldAABB();
                    result = Irufemi::Collision::GetCollisionResult(boxB, sphereA);
                    result.normal = Irufemi::Math::Multiply(-1.0f, result.normal);
                } else if (colB->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
                    Irufemi::Sphere sphereB = static_cast<SphereColliderComponent*>(colB)->GetWorldSphere();
                    result = Irufemi::Collision::GetCollisionResult(sphereA, sphereB);
                } else if (colB->GetColliderType() == ColliderComponent::ColliderType::OBB) {
                    Irufemi::OBB obbB = static_cast<OBBColliderComponent*>(colB)->GetWorldOBB();
                    result = Irufemi::Collision::GetCollisionResult(obbB, sphereA);
                    result.normal = Irufemi::Math::Multiply(-1.0f, result.normal);
                }
            } else if (colA->GetColliderType() == ColliderComponent::ColliderType::OBB) {
                Irufemi::OBB obbA = static_cast<OBBColliderComponent*>(colA)->GetWorldOBB();

                if (colB->GetColliderType() == ColliderComponent::ColliderType::AABB) {
                    Irufemi::AABB boxB = static_cast<AABBColliderComponent*>(colB)->GetWorldAABB();
                    result = Irufemi::Collision::GetCollisionResult(obbA, boxB);
                } else if (colB->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
                    Irufemi::Sphere sphereB = static_cast<SphereColliderComponent*>(colB)->GetWorldSphere();
                    result = Irufemi::Collision::GetCollisionResult(obbA, sphereB);
                } else if (colB->GetColliderType() == ColliderComponent::ColliderType::OBB) {
                    Irufemi::OBB obbB = static_cast<OBBColliderComponent*>(colB)->GetWorldOBB();
                    result = Irufemi::Collision::GetCollisionResult(obbA, obbB);
                }
            }

            if (result.isHit) {
                currentCollisions.insert(pairKey);

                // --- コールバック呼び出し (Enter / Stay) ---
                if (previousCollisions_.find(pairKey) == previousCollisions_.end()) {
                    // 新規衝突 (Enter)
                    if (result.isHit) {
                        if (colA->onCollisionEnter_) {
                            colA->onCollisionEnter_(colB);
                        }
                        if (colB->onCollisionEnter_) {
                            colB->onCollisionEnter_(colA);
                        }

                        if (colA->GetGameObject()) {
                            colA->GetGameObject()->SendCollisionEnter(colB->GetGameObject());
                        }
                        if (colB->GetGameObject()) {
                            colB->GetGameObject()->SendCollisionEnter(colA->GetGameObject());
                        }
                    }
                } else {
                    // 継続衝突 (Stay)
                    if (colA->onCollisionStay_) {
                        colA->onCollisionStay_(colB);
                    }
                    if (colB->onCollisionStay_) {
                        colB->onCollisionStay_(colA);
                    }

                    if (colA->GetGameObject()) {
                        colA->GetGameObject()->SendCollisionStay(colB->GetGameObject());
                    }
                    if (colB->GetGameObject()) {
                        colB->GetGameObject()->SendCollisionStay(colA->GetGameObject());
                    }
                }

                // --- 押し戻し処理 (Kinematic Resolution) ---
                if (!colA->isTrigger_ && !colB->isTrigger_) {
                    TransformComponent* transformA =
                        colA->GetGameObject() ? colA->GetGameObject()->GetComponent<TransformComponent>() : nullptr;
                    TransformComponent* transformB =
                        colB->GetGameObject() ? colB->GetGameObject()->GetComponent<TransformComponent>() : nullptr;

                    bool canMoveA = transformA && !colA->isStatic_;
                    bool canMoveB = transformB && !colB->isStatic_;

                    if (canMoveA && canMoveB) {
                        // 両方動く場合は半分の距離ずつ押し戻す
                        Irufemi::Vector3 pushA = Irufemi::Math::Multiply(result.depth * 0.5f, result.normal);
                        Irufemi::Vector3 pushB =
                            Irufemi::Math::Multiply(result.depth * 0.5f, Irufemi::Math::Multiply(-1.0f, result.normal));

                        pushA.x *= colA->pushbackMask_.x;
                        pushA.y *= colA->pushbackMask_.y;
                        pushA.z *= colA->pushbackMask_.z;

                        pushB.x *= colB->pushbackMask_.x;
                        pushB.y *= colB->pushbackMask_.y;
                        pushB.z *= colB->pushbackMask_.z;

                        transformA->SetWorldPosition(Irufemi::Math::Add(transformA->GetWorldPosition(), pushA));
                        transformB->SetWorldPosition(Irufemi::Math::Add(transformB->GetWorldPosition(), pushB));
                    } else if (canMoveA) {
                        Irufemi::Vector3 pushA = Irufemi::Math::Multiply(result.depth, result.normal);
                        
                        pushA.x *= colA->pushbackMask_.x;
                        pushA.y *= colA->pushbackMask_.y;
                        pushA.z *= colA->pushbackMask_.z;
                        
                        transformA->SetWorldPosition(Irufemi::Math::Add(transformA->GetWorldPosition(), pushA));
                    } else if (canMoveB) {
                        Irufemi::Vector3 pushB =
                            Irufemi::Math::Multiply(result.depth, Irufemi::Math::Multiply(-1.0f, result.normal));
                            
                        pushB.x *= colB->pushbackMask_.x;
                        pushB.y *= colB->pushbackMask_.y;
                        pushB.z *= colB->pushbackMask_.z;
                        
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

            if (colA && colA->onCollisionExit_) {
                colA->onCollisionExit_(colB);
            }
            if (colB && colB->onCollisionExit_) {
                colB->onCollisionExit_(colA);
            }

            if (colA && colA->GetGameObject()) {
                colA->GetGameObject()->SendCollisionExit(colB ? colB->GetGameObject() : nullptr);
            }
            if (colB && colB->GetGameObject()) {
                colB->GetGameObject()->SendCollisionExit(colA ? colA->GetGameObject() : nullptr);
            }
        }
    }

    // 更新
    previousCollisions_ = std::move(currentCollisions);
}

void CollisionManager::DrawDebug(GameObject* selectedObject) {
    if (!debugLine_) {
        return;
    }

    debugLine_->ClearInstances();

    for (ColliderComponent* collider : colliders_) {
        if (!collider) {
            continue;
        }
        auto go = collider->GetGameObject();
        if (!go) {
            continue;
        }
        if (reinterpret_cast<uintptr_t>(go) < 0x1000) {
            Log::OutPutLog(std::cerr, "[CollisionManager] CRITICAL ERROR: Caught invalid GameObject pointer (0x" +
                                          std::format("{:X}", reinterpret_cast<uintptr_t>(go)) +
                                          ") in CollisionManager!\n");
            continue;
        }
        if (!go->GetIsActive()) {
            continue;
        }

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
        if (!isDrawDebugLine_ && !isSelected) {
            continue;
        }

        Irufemi::Vector4 color =
            isSelected ? Irufemi::Vector4{1.0f, 0.5f, 0.0f, 1.0f} : Irufemi::Vector4{0.0f, 1.0f, 0.0f, 1.0f};

        if (collider->GetColliderType() == ColliderComponent::ColliderType::AABB) {
            AABBColliderComponent* aabbCol = static_cast<AABBColliderComponent*>(collider);
            Irufemi::AABB aabb = aabbCol->GetWorldAABB();

            Irufemi::Vector3 size = {aabb.max.x - aabb.min.x, aabb.max.y - aabb.min.y, aabb.max.z - aabb.min.z};
            Irufemi::Vector3 center = {(aabb.max.x + aabb.min.x) * 0.5f, (aabb.max.y + aabb.min.y) * 0.5f,
                                       (aabb.max.z + aabb.min.z) * 0.5f};
            Irufemi::Matrix4x4 transform = Irufemi::Math::MakeAffineMatrix(size, Irufemi::Vector3{0, 0, 0}, center);

            if (debugPrimitiveRenderer_) {
                debugPrimitiveRenderer_->AddCube(transform, color);
            }
        } else if (collider->GetColliderType() == ColliderComponent::ColliderType::Sphere) {
            SphereColliderComponent* sphereCol = static_cast<SphereColliderComponent*>(collider);
            Irufemi::Sphere sphere = sphereCol->GetWorldSphere();

            if (debugPrimitiveRenderer_) {
                debugPrimitiveRenderer_->AddSphere(sphere.center, sphere.radius, color);
            }
        } else if (collider->GetColliderType() == ColliderComponent::ColliderType::OBB) {
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

bool CollisionManager::Raycast(const Irufemi::Ray& ray, RaycastHit& hitInfo, float maxDistance, uint32_t layerMask,
                               GameObject* ignoreObject) {
    hitInfo.isHit = false;
    hitInfo.distance = maxDistance;

    std::shared_lock<std::shared_mutex> lock(collidersMutex_);

    std::vector<ColliderComponent*> potentialHits;
    dynamicBVH_.RaycastQuery(ray, maxDistance, potentialHits);

    for (ColliderComponent* collider : potentialHits) {
        if (!collider) {
            continue;
        }
        auto go = collider->GetGameObject();
        if (!go) {
            continue;
        }
        if (reinterpret_cast<uintptr_t>(go) < 0x1000) {
            Log::OutPutLog(std::cerr, "[CollisionManager] CRITICAL ERROR: Caught invalid GameObject pointer (0x" +
                                          std::format("{:X}", reinterpret_cast<uintptr_t>(go)) +
                                          ") in CollisionManager!\n");
            continue;
        }
        if (!go->GetIsActive()) {
            continue;
        }

        // 除外オブジェクトならスキップ
        if (ignoreObject && collider->GetGameObject() == ignoreObject) {
            continue;
        }

        // 指定されたレイヤーマスクに合致するか判定
        if ((collider->layer_ & layerMask) == 0) {
            continue;
        }

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

void CollisionManager::QueryAABB(const Irufemi::AABB& aabb, std::vector<ColliderComponent*>& outHits) const {
    std::shared_lock<std::shared_mutex> lock(collidersMutex_);
    dynamicBVH_.Query(aabb, outHits);
}

void CollisionManager::DrawDebugRay(const Irufemi::Ray& ray, float distance, const Irufemi::Vector4& color) {
    debugRays_.push_back({ray, distance, color});
}

void CollisionManager::DrawDebugAABB(const Irufemi::AABB& aabb, const Irufemi::Vector4& color) {
    if (debugPrimitiveRenderer_) {
        Irufemi::Vector3 center = (aabb.min + aabb.max) * 0.5f;
        Irufemi::Vector3 size = aabb.max - aabb.min;
        Irufemi::Matrix4x4 transform = Irufemi::Math::MakeAffineMatrix(size, Irufemi::Vector3::zero, center);
        debugPrimitiveRenderer_->AddCube(transform, color);
    }
}

std::future<std::pair<bool, RaycastHit>> CollisionManager::RaycastAsync(ThreadPool* pool, const Irufemi::Ray& ray,
                                                                        float maxDistance, uint32_t layerMask,
                                                                        GameObject* ignoreObject) {
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
