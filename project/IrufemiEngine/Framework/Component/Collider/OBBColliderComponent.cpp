#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Physics/CollisionManager.h"
#include "Core/Math/MathFunction.h"

OBBColliderComponent::OBBColliderComponent() {}

void OBBColliderComponent::DrawDebug() {}

void OBBColliderComponent::OnRegisterProperties() {
    ColliderComponent::OnRegisterProperties();
    RegisterProperty("Local Offset X", &localOffset_.x);
    RegisterProperty("Local Offset Y", &localOffset_.y);
    RegisterProperty("Local Offset Z", &localOffset_.z);
    RegisterProperty("Local Size X", &localSize_.x);
    RegisterProperty("Local Size Y", &localSize_.y);
    RegisterProperty("Local Size Z", &localSize_.z);
}

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
    return GetWorldOBB().ToAABB();
}

nlohmann::json OBBColliderComponent::Serialize() {
    nlohmann::json j = ColliderComponent::Serialize();
    j["localOffset"] = {localOffset_.x, localOffset_.y, localOffset_.z};
    j["localSize"] = {localSize_.x, localSize_.y, localSize_.z};
    return j;
}

void OBBColliderComponent::Deserialize(const nlohmann::json& j) {
    ColliderComponent::Deserialize(j);
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
}

std::shared_ptr<Component> OBBColliderComponent::Clone() {
    auto clone = std::make_shared<OBBColliderComponent>();
    clone->CopyPropertiesFrom(this);
    clone->localOffset_ = this->localOffset_;
    clone->localSize_ = this->localSize_;
    return clone;
}
