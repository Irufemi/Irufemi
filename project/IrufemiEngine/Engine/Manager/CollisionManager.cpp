#include "CollisionManager.h"
#include "Framework/Component/Collider/ColliderComponent.h"
#include "Framework/Component/Collider/AABBColliderComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Renderer/LineInstanced/LineClass.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <imgui.h>
#include "Engine/Core/Math/MathFunction.h"


void CollisionManager::Initialize() {
    if (!debugLine_) {
        debugLine_ = std::make_unique<Line3DRegion>();
        debugLine_->Initialize();
    }
    
    layerNames_ = { "Default" };
    LoadLayers(layerConfigFilePath_);
}

CollisionManager::~CollisionManager() = default;

void CollisionManager::Clear() {
    colliders_.clear();
    previousCollisions_.clear();
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

    // 削除されるコライダーが含まれているペアをpreviousCollisions_から削除し、Exitを呼ぶ
    for (auto iter = previousCollisions_.begin(); iter != previousCollisions_.end(); ) {
        if (iter->first == collider || iter->second == collider) {
            ColliderComponent* other = (iter->first == collider) ? iter->second : iter->first;
            
            // 削除される側からExitを呼ぶ（任意）
            if (collider->onCollisionExit_) collider->onCollisionExit_(other);
            if (other && other->onCollisionExit_) other->onCollisionExit_(collider);
            
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
            for (size_t j = i + 1; j < colliders_.size(); ++j) {
                ColliderComponent* colA = colliders_[i];
                ColliderComponent* colB = colliders_[j];

                if (!colA || !colB) continue;

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
                    } else {
                        // 継続衝突 (Stay)
                        if (colA->onCollisionStay_) colA->onCollisionStay_(colB);
                        if (colB->onCollisionStay_) colB->onCollisionStay_(colA);
                    }

                    // --- 押し戻し処理 (Kinematic Resolution) ---
                    if (!colA->isTrigger_ && !colB->isTrigger_) {
                        TransformComponent* transformA = colA->GetGameObject() ? colA->GetGameObject()->GetComponent<TransformComponent>() : nullptr;
                        TransformComponent* transformB = colB->GetGameObject() ? colB->GetGameObject()->GetComponent<TransformComponent>() : nullptr;

                        if (transformA && transformB) {
                            // 両方動く場合は半分の距離ずつ押し戻す
                            Vector3 pushA = Math::Multiply(result.depth * 0.5f, result.normal);
                            Vector3 pushB = Math::Multiply(result.depth * 0.5f, Math::Multiply(-1.0f, result.normal));
                            
                            transformA->position_ = Math::Add(transformA->position_, pushA);
                            transformB->position_ = Math::Add(transformB->position_, pushB);
                        } else if (transformA) {
                            Vector3 pushA = Math::Multiply(result.depth, result.normal);
                            transformA->position_ = Math::Add(transformA->position_, pushA);
                        } else if (transformB) {
                            Vector3 pushB = Math::Multiply(result.depth, Math::Multiply(-1.0f, result.normal));
                            transformB->position_ = Math::Add(transformB->position_, pushB);
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
        }
    }

    // 更新
    previousCollisions_ = std::move(currentCollisions);
}

void CollisionManager::DrawDebug() {
    if (!debugLine_) return;
    
    debugLine_->ClearInstances();
    
    if (isDrawDebugLine_) {
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
            Vector4 color = { 0.0f, 1.0f, 0.0f, 1.0f }; // 緑
            
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
            Vector4 color = { 0.0f, 1.0f, 0.0f, 1.0f };
            
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
    } // end if isDrawDebugLine_
    
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

void CollisionManager::DrawLayerInspectorGUI(uint32_t& layer, uint32_t& mask) {
#ifdef EditorMode
    ImGui::Separator();
    ImGui::Text("Collision Settings");

    // レイヤー選択 (ComboBox)
    int currentLayerIndex = 0;
    for (int i = 0; i < layerNames_.size(); ++i) {
        if (layer == (1u << i)) {
            currentLayerIndex = i;
            break;
        }
    }

    if (ImGui::BeginCombo("Layer", layerNames_[currentLayerIndex].c_str())) {
        for (int i = 0; i < layerNames_.size(); ++i) {
            bool isSelected = (currentLayerIndex == i);
            if (ImGui::Selectable(layerNames_[i].c_str(), isSelected)) {
                layer = (1u << i);
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // マスク選択 (TreeNode内にCheckBoxリスト)
    if (ImGui::TreeNode("Collision Mask")) {
        // すべて選択/解除ボタン
        if (ImGui::Button("All")) mask = 0xFFFFFFFF;
        ImGui::SameLine();
        if (ImGui::Button("None")) mask = 0;

        for (int i = 0; i < layerNames_.size(); ++i) {
            bool isMasked = (mask & (1u << i)) != 0;
            if (ImGui::Checkbox(layerNames_[i].c_str(), &isMasked)) {
                if (isMasked) mask |= (1u << i);
                else          mask &= ~(1u << i);
            }
        }
        ImGui::TreePop();
    }

    // レイヤーの編集UI
    if (ImGui::Button("Edit Layers...")) {
        ImGui::OpenPopup("Edit Layers");
    }

    if (ImGui::BeginPopupModal("Edit Layers", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Manage Collision Layers");
        ImGui::Separator();

        for (int i = 0; i < layerNames_.size(); ++i) {
            ImGui::PushID(i);
            ImGui::Text("%2d: ", i);
            ImGui::SameLine();
            
            if (i == 0) {
                // Defaultレイヤーは変更不可
                ImGui::TextDisabled("%s (Fixed)", layerNames_[i].c_str());
            } else {
                char nameBuffer[64];
                strncpy_s(nameBuffer, sizeof(nameBuffer), layerNames_[i].c_str(), _TRUNCATE);
                
                ImGui::SetNextItemWidth(150);
                if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer))) {
                    layerNames_[i] = nameBuffer;
                    SaveLayers(layerConfigFilePath_);
                }
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    RemoveLayer(i);
                    // 削除したのでインデックスがずれるため一旦抜ける
                    ImGui::PopID();
                    break; 
                }
            }
            ImGui::PopID();
        }

        if (layerNames_.size() < 32) {
            if (ImGui::Button("+ Add New Layer")) {
                AddLayer("New Layer");
            }
        } else {
            ImGui::TextDisabled("Max layers reached (32).");
        }

        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
#endif
}

bool CollisionManager::Raycast(const Ray& ray, RaycastHit& hitInfo, float maxDistance, uint32_t layerMask, GameObject* ignoreObject) {
    hitInfo.isHit = false;
    hitInfo.distance = maxDistance;

    for (ColliderComponent* collider : colliders_) {
        if (!collider) continue;

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
