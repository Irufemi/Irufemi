#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Core/Math/MathFunction.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Physics/CollisionManager.h"

OBBColliderComponent::OBBColliderComponent() {}

OBBColliderComponent::~OBBColliderComponent() {
    if (collisionManager_)
        collisionManager_->UnregisterCollider(this);
}

void OBBColliderComponent::Initialize() {
    if (gameObject_) {
    }
    // 初期化時にCollisionManagerに自身を登録する
    if (collisionManager_)
        collisionManager_->RegisterCollider(this);
}

void OBBColliderComponent::Update() {
    if (!GetTransform() && gameObject_) {
    }
}

void OBBColliderComponent::DrawDebug() {}

Irufemi::OBB OBBColliderComponent::GetWorldOBB() const {
    Irufemi::OBB obb;
    if (GetTransform()) {
        Irufemi::Vector3 worldPos = GetTransform()->GetWorldPosition();
        Irufemi::Vector3 worldScale = GetTransform()->GetWorldScale();

        obb.orientations[0] = GetTransform()->GetWorldRight();
        obb.orientations[1] = GetTransform()->GetWorldUp();
        obb.orientations[2] = GetTransform()->GetWorldForward();

        // Offsetも回転・スケールを考慮
        obb.center = worldPos + obb.orientations[0] * (localOffset_.x * worldScale.x) +
                     obb.orientations[1] * (localOffset_.y * worldScale.y) +
                     obb.orientations[2] * (localOffset_.z * worldScale.z);

        obb.size = {localSize_.x * worldScale.x, localSize_.y * worldScale.y, localSize_.z * worldScale.z};
    } else {
        obb.center = localOffset_;
        obb.orientations[0] = {1.0f, 0.0f, 0.0f};
        obb.orientations[1] = {0.0f, 1.0f, 0.0f};
        obb.orientations[2] = {0.0f, 0.0f, 1.0f};
        obb.size = localSize_;
    }
    return obb;
}

Irufemi::AABB OBBColliderComponent::GetBoundingBox() const {
    Irufemi::OBB obb = GetWorldOBB();
    Irufemi::AABB aabb;
    // OBBを包含するAABBの半径(各軸ごとの最大投影長)を計算
    Irufemi::Vector3 extents;
    extents.x = std::abs(obb.orientations[0].x * obb.size.x) + std::abs(obb.orientations[1].x * obb.size.y) +
                std::abs(obb.orientations[2].x * obb.size.z);
    extents.y = std::abs(obb.orientations[0].y * obb.size.x) + std::abs(obb.orientations[1].y * obb.size.y) +
                std::abs(obb.orientations[2].y * obb.size.z);
    extents.z = std::abs(obb.orientations[0].z * obb.size.x) + std::abs(obb.orientations[1].z * obb.size.y) +
                std::abs(obb.orientations[2].z * obb.size.z);

    aabb.min = {obb.center.x - extents.x, obb.center.y - extents.y, obb.center.z - extents.z};
    aabb.max = {obb.center.x + extents.x, obb.center.y + extents.y, obb.center.z + extents.z};
    return aabb;
}

nlohmann::json OBBColliderComponent::Serialize() {
    nlohmann::json j;
    j["localOffset"] = {localOffset_.x, localOffset_.y, localOffset_.z};
    j["localSize"] = {localSize_.x, localSize_.y, localSize_.z};
    j["layer"] = layer_;
    j["mask"] = mask_;
    return j;
}

void OBBColliderComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("localOffset")) {
        localOffset_.x = j["localOffset"][0];
        localOffset_.y = j["localOffset"][1];
        localOffset_.z = j["localOffset"][2];
    } else if (j.contains("center")) {
        localOffset_.x = j["center"][0];
        localOffset_.y = j["center"][1];
        localOffset_.z = j["center"][2];
    }

    if (j.contains("localSize")) {
        localSize_.x = j["localSize"][0];
        localSize_.y = j["localSize"][1];
        localSize_.z = j["localSize"][2];
    } else if (j.contains("size")) {
        localSize_.x = j["size"][0];
        localSize_.y = j["size"][1];
        localSize_.z = j["size"][2];
    }
    if (j.contains("layer"))
        layer_ = j["layer"];
    if (j.contains("mask"))
        mask_ = j["mask"];
}

std::shared_ptr<Component> OBBColliderComponent::Clone() {
    auto clone = std::make_shared<OBBColliderComponent>();
    clone->CopyPropertiesFrom(this);
    clone->localOffset_ = this->localOffset_;
    clone->localSize_ = this->localSize_;
    return clone;
}
