#include "RailRelativeFollowerComponent.h"
#include "SplineFollowerComponent.h"
#include "Framework/Component/Utility/SplineComponent.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Engine/Core/Math/MathFunction.h"
#include <cmath>

void RailRelativeFollowerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    RegisterProperty("Target Object Name", &targetObjectName_);
    RegisterProperty("Distance Offset", &distanceOffset_);
    RegisterProperty("Local Offset X", &localOffset_.x);
    RegisterProperty("Local Offset Y", &localOffset_.y);
    RegisterProperty("Local Offset Z", &localOffset_.z);
}

void RailRelativeFollowerComponent::Initialize() {
}

void RailRelativeFollowerComponent::Start() {
    if (!gameObject_) return;
    auto scene = gameObject_->GetScene();
    if (scene) {
        // 名前でターゲットオブジェクトを検索
        auto target = scene->FindGameObject(targetObjectName_);
        if (target) {
            targetObject_ = target;
            targetFollower_ = target->GetComponent<SplineFollowerComponent>();
            if (targetFollower_) {
                cachedPath_ = targetFollower_->GetCachedPath();
            }
        }
    }
}

void RailRelativeFollowerComponent::Update() {
    if (!gameObject_) return;

    // ターゲットまたはパスが未解決の場合は再取得を試みる
    if (!targetFollower_ || !cachedPath_) {
        // targetObject_ が既に取得できている場合は FindGameObject (Start) の呼び出しを避ける
        if (auto target = targetObject_.lock()) {
            if (!targetFollower_) {
                targetFollower_ = target->GetComponent<SplineFollowerComponent>();
            }
            if (targetFollower_ && !cachedPath_) {
                cachedPath_ = targetFollower_->GetCachedPath();
            }
            if (!targetFollower_ || !cachedPath_) return;
        } else {
            // FindGameObject はシーンのミューテックスをロックするため、
            // 毎フレーム複数スレッドから呼ばれると深刻なスレッド競合（ガタつき）の原因になる。
            // そのためリトライ頻度を落とす。
            static thread_local int s_retryCounter = 0;
            if (++s_retryCounter < 30) {
                return;
            }
            s_retryCounter = 0;

            Start(); 
            if (!targetFollower_ || !cachedPath_) return; // それでも見つからなければ終了
        }
    }

    // ターゲットの現在のレール上での距離を取得
    float baseDistance = targetFollower_->GetCurrentDistance();
    
    // 自身が位置すべきレール上の距離を計算
    float targetDistance = baseDistance + distanceOffset_;
    float totalLength = cachedPath_->GetTotalLength();
    
    // 距離のクランプ（レールの終端を超えないようにする）
    if (targetDistance > totalLength) targetDistance = totalLength;
    if (targetDistance < 0.0f) targetDistance = 0.0f;

    // レール上の座標と接線（進行方向）を取得
    Vector3 basePos = cachedPath_->GetPointAtDistance(targetDistance);
    Vector3 tangent = cachedPath_->GetTangentAtDistance(targetDistance);
    
    // 基本となる回転（Z前方）を計算
    float yaw = std::atan2(tangent.x, tangent.z);
    float pitch = std::asin(-tangent.y);
    Vector3 rotation = {pitch, yaw, 0.0f};

    // XYローカルオフセットの適用（レール中心から上下左右へのズレ）
    Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(rotation);
    Vector3 right = { rotMat.m[0][0], rotMat.m[0][1], rotMat.m[0][2] };
    Vector3 up    = { rotMat.m[1][0], rotMat.m[1][1], rotMat.m[1][2] };
    Vector3 forward={ rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2] };

    Vector3 finalPos = basePos;
    finalPos = Math::Add(finalPos, Math::Multiply(localOffset_.x, right));
    finalPos = Math::Add(finalPos, Math::Multiply(localOffset_.y, up));
    finalPos = Math::Add(finalPos, Math::Multiply(localOffset_.z, forward));

    auto transform = gameObject_->GetComponent<TransformComponent>();
    if (transform) {
        transform->SetPosition(finalPos);
        transform->SetRotation(rotation);
    }
}
