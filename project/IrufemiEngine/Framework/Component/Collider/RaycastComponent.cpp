#include "Framework/Component/Collider/RaycastComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Core/Math/MathFunction.h"

void RaycastComponent::Initialize() {
    if (gameObject_) {
    }
}

void RaycastComponent::Update() {
    if (!GetTransform() && gameObject_) {
    }

    if (GetTransform()) {
        // ワールド空間でのレイの起点と方向を計算
        Irufemi::Vector3 worldPos = GetTransform()->GetWorldPosition();
        Irufemi::Matrix4x4 worldMat = GetTransform()->GetWorldMatrix();

        // オフセットの適用
        Irufemi::Vector3 worldOffset = Irufemi::Math::TransformNormal(localOffset_, worldMat);
        currentRay_.origin = worldPos + worldOffset;

        // 方向の適用（ローカル方向ベクトルをワールドへ回転）
        Irufemi::Vector3 worldDir = Irufemi::Math::TransformNormal(localDirection_, worldMat);
        worldDir = Irufemi::Math::Normalize(worldDir);
        currentRay_.diff = worldDir; // diffを方向として扱う

        // 判定実行（自分自身が持つ他のコライダーには当たらないようにgameObject_を渡す）
        collisionManager_->Raycast(currentRay_, hitInfo_, maxDistance_, mask_, gameObject_);

        if (hitInfo_.isHit && onHit_) {
            onHit_(hitInfo_);
        }

        // 毎フレーム判定後にデバッグ描画を行う
        DrawDebug();
    }
}

void RaycastComponent::DrawDebug() {
    if (showDebugLine_) {
        Irufemi::Vector4 color =
            hitInfo_.isHit ? Irufemi::Vector4{1.0f, 0.0f, 0.0f, 1.0f} : Irufemi::Vector4{0.0f, 1.0f, 0.0f, 1.0f};
        float drawDist = hitInfo_.isHit ? hitInfo_.distance : maxDistance_;
        if (collisionManager_)
            collisionManager_->DrawDebugRay(currentRay_, drawDist, color);
    }
}

nlohmann::json RaycastComponent::Serialize() {
    nlohmann::json j;
    j["localOffset"] = {localOffset_.x, localOffset_.y, localOffset_.z};
    j["localDirection"] = {localDirection_.x, localDirection_.y, localDirection_.z};
    j["maxDistance"] = maxDistance_;
    j["mask"] = mask_;
    j["showDebugLine"] = showDebugLine_;
    return j;
}

void RaycastComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("localOffset")) {
        localOffset_.x = j["localOffset"][0];
        localOffset_.y = j["localOffset"][1];
        localOffset_.z = j["localOffset"][2];
    }
    if (j.contains("localDirection")) {
        localDirection_.x = j["localDirection"][0];
        localDirection_.y = j["localDirection"][1];
        localDirection_.z = j["localDirection"][2];
    }
    if (j.contains("maxDistance"))
        maxDistance_ = j["maxDistance"];
    if (j.contains("mask"))
        mask_ = j["mask"];
    if (j.contains("showDebugLine"))
        showDebugLine_ = j["showDebugLine"];
}

std::shared_ptr<Component> RaycastComponent::Clone() {
    auto clone = std::make_shared<RaycastComponent>();
    clone->CopyPropertiesFrom(this);
    clone->localOffset_ = this->localOffset_;
    clone->localDirection_ = this->localDirection_;
    clone->maxDistance_ = this->maxDistance_;
    clone->mask_ = this->mask_;
    clone->showDebugLine_ = this->showDebugLine_;
    return clone;
}
