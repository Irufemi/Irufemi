#include "SplineFollowerComponent.h"
#include "Framework/Component/Utility/SplineComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/BaseScene.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Renderer/Object/Line/LineClass.h"
#include <algorithm>
#include <cmath>

void SplineFollowerComponent::OnRegisterProperties() {
    RegisterProperty("Speed", &speed_);
    RegisterGameObjectRef("Target Path", &targetPathID_);
}

void SplineFollowerComponent::Initialize() {
    cachedPath_ = nullptr;
    currentDistance_ = 0.0f;
}

void SplineFollowerComponent::OnIDRemapped(const std::unordered_map<uint64_t, uint64_t>& idMap) {
    if (targetPathID_ != 0) {
        auto it = idMap.find(targetPathID_);
        if (it != idMap.end()) {
            targetPathID_ = it->second;
        }
    }
}

void SplineFollowerComponent::Start() {
    if (!gameObject_) return;
    auto* scene = gameObject_->GetScene();
    if (scene && targetPathID_ != 0) {
        if (auto obj = scene->FindGameObjectByID(targetPathID_)) {
            cachedPath_ = obj->GetComponent<SplineComponent>();
        }
    }
}

void SplineFollowerComponent::Update() {
    if (!gameObject_) return;

    float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (deltaTime <= 0.0f) {
        return;
    }

    if (cachedPath_ && !cachedPath_->GetWaypoints().empty()) {
        // 進行度を前進させる (m/s)
        float totalLength = cachedPath_->GetTotalLength();
        currentDistance_ += speed_ * deltaTime;
        if (currentDistance_ > totalLength) currentDistance_ = totalLength;

        // レール上の座標と接線（進行方向）を距離ベースで取得
        Irufemi::Vector3 basePos = cachedPath_->GetPointAtDistance(currentDistance_);
        Irufemi::Vector3 tangent = cachedPath_->GetTangentAtDistance(currentDistance_);

        auto transform = GetTransform();
        if (transform) {
            transform->SetWorldPosition(basePos);
            
            // 進行方向に向くように回転を設定 (Z前方)
            float yaw = std::atan2(tangent.x, tangent.z);
            float pitch = std::asin(-tangent.y);
            transform->SetWorldRotation({pitch, yaw, 0.0f});
        }
    }
}

void SplineFollowerComponent::Draw() {
}
