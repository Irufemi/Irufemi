#include "Framework/Component/Collider/AABBColliderComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Physics/CollisionManager.h"


AABBColliderComponent::AABBColliderComponent() {}

AABBColliderComponent::~AABBColliderComponent() {
    if (collisionManager_) collisionManager_->UnregisterCollider(this);
}

void AABBColliderComponent::Initialize() {
    if (gameObject_) {
    }
    // 初期化時にCollisionManagerに自身を登録する
    if (collisionManager_) collisionManager_->RegisterCollider(this);
}

void AABBColliderComponent::Update() {
    if (!GetTransform() && gameObject_) {
    }
}

void AABBColliderComponent::DrawDebug() {
}



Irufemi::AABB AABBColliderComponent::GetWorldAABB() const {
    Irufemi::AABB aabb;
    if (GetTransform()) {
        Irufemi::Vector3 worldPos = GetTransform()->GetWorldPosition();
        Irufemi::Vector3 worldScale = GetTransform()->GetWorldScale();
        
        // オブジェクトの回転とスケールを考慮したワールド空間のローカルオフセット
        Irufemi::Vector3 worldOffset = 
            GetTransform()->GetWorldRight() * (localOffset_.x * worldScale.x) +
            GetTransform()->GetWorldUp() * (localOffset_.y * worldScale.y) +
            GetTransform()->GetWorldForward() * (localOffset_.z * worldScale.z);
            
        Irufemi::Vector3 center = worldPos + worldOffset;
        
        // ローカル軸ごとのサイズベクトルをワールド空間に変換
        Irufemi::Vector3 rightSize = GetTransform()->GetWorldRight() * (localSize_.x * worldScale.x);
        Irufemi::Vector3 upSize = GetTransform()->GetWorldUp() * (localSize_.y * worldScale.y);
        Irufemi::Vector3 forwardSize = GetTransform()->GetWorldForward() * (localSize_.z * worldScale.z);
        
        // 各ワールド軸（X, Y, Z）への射影の絶対値の和がAABBのサイズ（extent）になる
        Irufemi::Vector3 extent;
        extent.x = std::abs(rightSize.x) + std::abs(upSize.x) + std::abs(forwardSize.x);
        extent.y = std::abs(rightSize.y) + std::abs(upSize.y) + std::abs(forwardSize.y);
        extent.z = std::abs(rightSize.z) + std::abs(upSize.z) + std::abs(forwardSize.z);
        
        aabb.min = { center.x - extent.x, center.y - extent.y, center.z - extent.z };
        aabb.max = { center.x + extent.x, center.y + extent.y, center.z + extent.z };
    } else {
        aabb.min = { localOffset_.x - localSize_.x, localOffset_.y - localSize_.y, localOffset_.z - localSize_.z };
        aabb.max = { localOffset_.x + localSize_.x, localOffset_.y + localSize_.y, localOffset_.z + localSize_.z };
    }
    return aabb;
}

nlohmann::json AABBColliderComponent::Serialize() {
    nlohmann::json j;
    j["localOffset"] = { localOffset_.x, localOffset_.y, localOffset_.z };
    j["localSize"] = { localSize_.x, localSize_.y, localSize_.z };
    j["layer"] = layer_;
    j["mask"] = mask_;
    return j;
}

void AABBColliderComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("localOffset")) {
        localOffset_.x = j["localOffset"][0];
        localOffset_.y = j["localOffset"][1];
        localOffset_.z = j["localOffset"][2];
    }
    if (j.contains("localSize")) {
        localSize_.x = j["localSize"][0];
        localSize_.y = j["localSize"][1];
        localSize_.z = j["localSize"][2];
    }
    if (j.contains("layer")) layer_ = j["layer"];
    if (j.contains("mask")) mask_ = j["mask"];
}
