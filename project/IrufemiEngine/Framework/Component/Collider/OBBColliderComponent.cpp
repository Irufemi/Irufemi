#include "OBBColliderComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/Manager/CollisionManager.h"
#include "Engine/Core/Math/MathFunction.h"


OBBColliderComponent::OBBColliderComponent() {}

OBBColliderComponent::~OBBColliderComponent() {
    if (collisionManager_) collisionManager_->UnregisterCollider(this);
}

void OBBColliderComponent::Initialize() {
    if (gameObject_) {
        transform_ = gameObject_->GetComponent<TransformComponent>();
    }
    // 初期化時にCollisionManagerに自身を登録する
    if (collisionManager_) collisionManager_->RegisterCollider(this);
}

void OBBColliderComponent::Update() {
    if (!transform_ && gameObject_) {
        transform_ = gameObject_->GetComponent<TransformComponent>();
    }
}

void OBBColliderComponent::DrawDebug() {
}



OBB OBBColliderComponent::GetWorldOBB() const {
    OBB obb;
    if (transform_) {
        Vector3 worldPos = transform_->GetWorldPosition();
        Vector3 worldScale = transform_->GetWorldScale();

        obb.orientations[0] = transform_->GetWorldRight();
        obb.orientations[1] = transform_->GetWorldUp();
        obb.orientations[2] = transform_->GetWorldForward();
        
        // Offsetも回転・スケールを考慮
        obb.center = worldPos 
                   + obb.orientations[0] * (localOffset_.x * worldScale.x)
                   + obb.orientations[1] * (localOffset_.y * worldScale.y)
                   + obb.orientations[2] * (localOffset_.z * worldScale.z);
                   
        obb.size = { localSize_.x * worldScale.x, localSize_.y * worldScale.y, localSize_.z * worldScale.z };
    } else {
        obb.center = localOffset_;
        obb.orientations[0] = {1.0f, 0.0f, 0.0f};
        obb.orientations[1] = {0.0f, 1.0f, 0.0f};
        obb.orientations[2] = {0.0f, 0.0f, 1.0f};
        obb.size = localSize_;
    }
    return obb;
}

nlohmann::json OBBColliderComponent::Serialize() {
    nlohmann::json j;
    j["localOffset"] = { localOffset_.x, localOffset_.y, localOffset_.z };
    j["localSize"] = { localSize_.x, localSize_.y, localSize_.z };
    j["layer"] = layer_;
    j["mask"] = mask_;
    return j;
}

void OBBColliderComponent::Deserialize(const nlohmann::json& j) {
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
