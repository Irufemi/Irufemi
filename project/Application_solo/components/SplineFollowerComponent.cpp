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
    RegisterProperty("Draw Debug Rail", &drawDebugRail_);
}

void SplineFollowerComponent::Initialize() {
    cachedPath_ = nullptr;
    progress_ = 0.0f;
    
    // デバッグ描画用のラインバッチの初期化
    debugLineBatch_ = std::make_unique<Line3DBatch>();
    debugLineBatch_->Initialize();
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
        // 進行度を前進させる
        progress_ += speed_ * deltaTime;
        if (progress_ > 1.0f) progress_ = 1.0f;

        // レール上の座標と接線（進行方向）を取得
        Vector3 basePos = cachedPath_->GetPointAt(progress_);
        Vector3 tangent = cachedPath_->GetTangentAt(progress_);

        auto transform = gameObject_->GetComponent<TransformComponent>();
        if (transform) {
            transform->SetPosition(basePos);
            
            // 進行方向に向くように回転を設定 (Z前方)
            float yaw = std::atan2(tangent.x, tangent.z);
            float pitch = std::asin(-tangent.y);
            transform->SetRotation({pitch, yaw, 0.0f});
        }
        
        // デバッグ描画の更新
        if (drawDebugRail_) {
            debugLineBatch_->ClearInstances();
            const int segments = 100;
            Vector4 color = {0.0f, 1.0f, 0.0f, 1.0f}; // 緑色
            Vector3 prevPos = cachedPath_->GetPointAt(0.0f);
            
            for (int i = 1; i <= segments; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(segments);
                Vector3 currentPos = cachedPath_->GetPointAt(t);
                debugLineBatch_->AddInstance(prevPos, currentPos, color);
                prevPos = currentPos;
            }
            debugLineBatch_->BuildInstanceBuffer();
            debugLineBatch_->Update();
        }
    }
}

void SplineFollowerComponent::Draw() {
    if (drawDebugRail_ && debugLineBatch_ && cachedPath_ && !cachedPath_->GetWaypoints().empty()) {
        debugLineBatch_->SyncBeforeDraw();
        debugLineBatch_->Draw();
    }
}
