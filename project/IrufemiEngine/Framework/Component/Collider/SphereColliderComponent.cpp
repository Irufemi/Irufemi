#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Physics/CollisionManager.h"

#include <algorithm>
#include <cmath>

SphereColliderComponent::SphereColliderComponent() {}

SphereColliderComponent::~SphereColliderComponent() {
    if (collisionManager_) {
        collisionManager_->UnregisterCollider(this);
    }
}

void SphereColliderComponent::OnRegisterProperties() {
    ColliderComponent::OnRegisterProperties();
    RegisterProperty("Local Offset", &localOffset_);
    RegisterProperty("Local Radius", &localRadius_);
    // ToDo: Layer や Mask も必要に応じて追加する
}

void SphereColliderComponent::Initialize() {
    if (gameObject_) {
    }
    if (collisionManager_) {
        collisionManager_->RegisterCollider(this);
    }
}

void SphereColliderComponent::Update() {
    if (!GetTransform() && gameObject_) {
    }
}

void SphereColliderComponent::DrawDebug() {}

Irufemi::Sphere SphereColliderComponent::GetWorldSphere() const {
    Irufemi::Sphere sphere;
    if (GetTransform()) {
        Irufemi::Vector3 worldPos = GetTransform()->GetWorldPosition();
        Irufemi::Vector3 worldScale = GetTransform()->GetWorldScale();

        // スケールの最大成分を半径に掛ける
        float scaleX = std::abs(worldScale.x);
        float scaleY = std::abs(worldScale.y);
        float scaleZ = std::abs(worldScale.z);

        // オブジェクトの回転とスケールを考慮したワールド空間のローカルオフセット
        Irufemi::Vector3 worldOffset = GetTransform()->GetWorldRight() * (localOffset_.x * scaleX) +
                                       GetTransform()->GetWorldUp() * (localOffset_.y * scaleY) +
                                       GetTransform()->GetWorldForward() * (localOffset_.z * scaleZ);

        float maxXY = scaleX > scaleY ? scaleX : scaleY;
        float maxScale = maxXY > scaleZ ? maxXY : scaleZ;

        sphere.center = worldPos + worldOffset;
        sphere.radius = localRadius_ * maxScale;
    } else {
        sphere.center = localOffset_;
        sphere.radius = localRadius_;
    }
    return sphere;
}

Irufemi::AABB SphereColliderComponent::GetBoundingBox() const {
    Irufemi::Sphere sphere = GetWorldSphere();
    Irufemi::AABB aabb;
    aabb.min = {sphere.center.x - sphere.radius, sphere.center.y - sphere.radius, sphere.center.z - sphere.radius};
    aabb.max = {sphere.center.x + sphere.radius, sphere.center.y + sphere.radius, sphere.center.z + sphere.radius};
    return aabb;
}

nlohmann::json SphereColliderComponent::Serialize() {
    nlohmann::json j;
    j["localOffset"] = {localOffset_.x, localOffset_.y, localOffset_.z};
    j["localRadius"] = localRadius_;
    j["layer"] = layer_;
    j["mask"] = mask_;
    return j;
}

void SphereColliderComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("localOffset")) {
        localOffset_.x = j["localOffset"][0];
        localOffset_.y = j["localOffset"][1];
        localOffset_.z = j["localOffset"][2];
    }
    if (j.contains("localRadius")) {
        localRadius_ = j["localRadius"];
    }
    if (j.contains("layer")) {
        layer_ = j["layer"];
    }
    if (j.contains("mask")) {
        mask_ = j["mask"];
    }
}

std::shared_ptr<Component> SphereColliderComponent::Clone() {
    auto clone = std::make_shared<SphereColliderComponent>();
    clone->CopyPropertiesFrom(this);
    clone->localOffset_ = this->localOffset_;
    clone->localRadius_ = this->localRadius_;
    return clone;
}
