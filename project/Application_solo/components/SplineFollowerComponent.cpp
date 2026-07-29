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
    RegisterProperty("Target Path Name", &targetPathName_);
}

void SplineFollowerComponent::Initialize() {
    cachedPath_ = nullptr;
    currentDistance_ = 0.0f;
}

void SplineFollowerComponent::Start() {
    if (!gameObject_) return;
    auto* scene = gameObject_->GetScene();
    if (scene) {
        // 名前で指定された対象パスを検索
        for (auto obj : scene->GetGameObjects()) {
            if (obj->GetName() == targetPathName_) {
                if (auto path = obj->GetComponent<SplineComponent>()) {
                    cachedPath_ = path;
                    break;
                }
            }
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
        Vector3 basePos = cachedPath_->GetPointAtDistance(currentDistance_);
        Vector3 tangent = cachedPath_->GetTangentAtDistance(currentDistance_);

        auto transform = gameObject_->GetComponent<TransformComponent>();
        if (transform) {
            transform->SetPosition(basePos);
            
            // 進行方向に向くように回転を設定 (Z前方)
            float yaw = std::atan2(tangent.x, tangent.z);
            float pitch = std::asin(-tangent.y);
            transform->SetRotation({pitch, yaw, 0.0f});
        }
    }
}

void SplineFollowerComponent::Draw() {
}
