#include "AABBColliderComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/Manager/CollisionManager.h"


AABBColliderComponent::AABBColliderComponent() {}

AABBColliderComponent::~AABBColliderComponent() {
    if (collisionManager_) collisionManager_->UnregisterCollider(this);
}

void AABBColliderComponent::Initialize() {
    if (gameObject_) {
        transform_ = gameObject_->GetComponent<TransformComponent>();
    }
    // 初期化時にCollisionManagerに自身を登録する
    if (collisionManager_) collisionManager_->RegisterCollider(this);
}

void AABBColliderComponent::Update() {
    if (!transform_ && gameObject_) {
        transform_ = gameObject_->GetComponent<TransformComponent>();
    }
}

void AABBColliderComponent::DrawDebug() {
}



Irufemi::AABB AABBColliderComponent::GetWorldAABB() const {
    Irufemi::AABB aabb;
    if (transform_) {
        Irufemi::Vector3 worldPos = transform_->GetWorldPosition();
        Irufemi::Vector3 worldScale = transform_->GetWorldScale();
        
        Irufemi::Vector3 center = { worldPos.x + localOffset_.x * worldScale.x, 
                           worldPos.y + localOffset_.y * worldScale.y, 
                           worldPos.z + localOffset_.z * worldScale.z };
        Irufemi::Vector3 scaledSize = { localSize_.x * worldScale.x, 
                               localSize_.y * worldScale.y, 
                               localSize_.z * worldScale.z };
        
        aabb.min = { center.x - scaledSize.x, center.y - scaledSize.y, center.z - scaledSize.z };
        aabb.max = { center.x + scaledSize.x, center.y + scaledSize.y, center.z + scaledSize.z };
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
