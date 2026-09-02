#include "Framework/Component/Camera/TargetFollowComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Scene/BaseScene.h"
#include "Core/System/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include <cmath>

void TargetFollowComponent::OnRegisterProperties() {
    RegisterGameObjectRef("Target Object", &targetObjectID_);
    RegisterProperty("Offset", &offset_);
    RegisterProperty("FollowDelay", &followDelay_);
}

void TargetFollowComponent::Initialize() {
    targetTransform_ = nullptr;
}

void TargetFollowComponent::OnIDRemapped(const std::unordered_map<uint64_t, uint64_t>& idMap) {
    if (targetObjectID_ != 0) {
        auto it = idMap.find(targetObjectID_);
        if (it != idMap.end()) {
            targetObjectID_ = it->second;
        }
    }
}

void TargetFollowComponent::Update() {
    if (!gameObject_) {
        return;
    }

    // 正確なデルタタイムの取得
    float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (deltaTime <= 0.0f) {
        deltaTime = 1.0f / 60.0f;
    }

    // ターゲットが未キャッシュの場合はシーン内から指定されたIDで探索
    if (!targetTransform_) {
        auto scene = gameObject_->GetScene();
        if (scene && targetObjectID_ != 0) {
            auto playerObj = scene->FindGameObjectByID(targetObjectID_);
            if (playerObj) {
                targetTransform_ = playerObj->GetComponent<TransformComponent>();
            }
        }
    }

    if (!targetTransform_) {
        return;
    }

    auto myTransform = GetTransform();
    if (!myTransform) {
        return;
    }

    // プレイヤーの向き（回転角度）から進行方向をベースとしたローカル座標系を作成
    float yaw = targetTransform_->GetWorldRotation().y;
    float pitch = targetTransform_->GetWorldRotation().x;

    // プレイヤーを基準とした回転行列の方向成分を計算
    Irufemi::Vector3 forward = {std::sin(yaw) * std::cos(pitch), std::sin(-pitch), std::cos(yaw) * std::cos(pitch)};

    // 正規化
    float len = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    if (len > 0.0001f) {
        forward.x /= len;
        forward.y /= len;
        forward.z /= len;
    } else {
        forward = {0.0f, 0.0f, 1.0f};
    }

    // 右ベクトルと上ベクトルの算出 (外積)
    Irufemi::Vector3 upVec = {0.0f, 1.0f, 0.0f};
    Irufemi::Vector3 right = {upVec.y * forward.z - upVec.z * forward.y, upVec.z * forward.x - upVec.x * forward.z,
                              upVec.x * forward.y - upVec.y * forward.x};
    float rLen = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
    if (rLen > 0.0001f) {
        right.x /= rLen;
        right.y /= rLen;
        right.z /= rLen;
    } else {
        right = {1.0f, 0.0f, 0.0f};
    }

    Irufemi::Vector3 up = {forward.y * right.z - forward.z * right.y, forward.z * right.x - forward.x * right.z,
                           forward.x * right.y - forward.y * right.x};

    // プレイヤー位置に、プレイヤーの向きに基づいたローカルオフセットを足す
    Irufemi::Vector3 targetCamPos = {
        targetTransform_->GetWorldPosition().x + right.x * offset_.x + up.x * offset_.y + forward.x * offset_.z,
        targetTransform_->GetWorldPosition().y + right.y * offset_.x + up.y * offset_.y + forward.y * offset_.z,
        targetTransform_->GetWorldPosition().z + right.z * offset_.x + up.z * offset_.y + forward.z * offset_.z};

    // 滑らかな追従 (線形補間/Lerp) を行う (フレームレート非依存)
    float t = 1.0f - std::pow(followDelay_, deltaTime);
    Irufemi::Vector3 currentPosWorld = myTransform->GetWorldPosition();
    Irufemi::Vector3 newPosWorld = currentPosWorld;
    newPosWorld.x += (targetCamPos.x - newPosWorld.x) * t;
    newPosWorld.y += (targetCamPos.y - newPosWorld.y) * t;
    newPosWorld.z += (targetCamPos.z - newPosWorld.z) * t;
    // カメラの向き（角度）もプレイヤーの向きに追従させる（Lerpで滑らかに旋回）
    Irufemi::Vector3 currentRotWorld = myTransform->GetWorldRotation();
    Irufemi::Vector3 newRotWorld = currentRotWorld;
    newRotWorld.x += (targetTransform_->GetWorldRotation().x - newRotWorld.x) * t;
    newRotWorld.y += (targetTransform_->GetWorldRotation().y - newRotWorld.y) * t;
    newRotWorld.z += (targetTransform_->GetWorldRotation().z - newRotWorld.z) * t;

    // 計算したワールド座標・回転からワールド行列を作成し、一度に設定する
    Irufemi::Matrix4x4 targetWorldMat = Irufemi::Math::MakeAffineMatrix({1, 1, 1}, newRotWorld, newPosWorld);
    myTransform->SetWorldMatrix(targetWorldMat);

    // 次のコンポーネント（CameraComponent等）のために強制更新
    myTransform->UpdateMatrixImmediate();
}
