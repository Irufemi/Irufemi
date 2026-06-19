#include "TargetFollowComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/BaseScene.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include <cmath>

void TargetFollowComponent::OnRegisterProperties() {
    RegisterProperty("Target Name", &targetName_);
    RegisterProperty("Offset", &offset_);
    RegisterProperty("FollowDelay", &followDelay_);
}

void TargetFollowComponent::Initialize() {
    targetTransform_ = nullptr;
}

void TargetFollowComponent::Update() {
    if (!gameObject_) return;

    // 正確なデルタタイムの取得
    float deltaTime = BaseModel::GetIrufemiEngine()->GetGameDeltaTime();
    if (deltaTime <= 0.0f) {
        deltaTime = 1.0f / 60.0f;
    }

    // ターゲットが未キャッシュの場合はシーン内から指定された名前で探索
    if (!targetTransform_) {
        auto scene = gameObject_->GetScene();
        if (scene && !targetName_.empty()) {
            auto playerObj = scene->FindGameObject(targetName_);
            if (playerObj) {
                targetTransform_ = playerObj->GetComponent<TransformComponent>();
            }
        }
    }

    if (!targetTransform_) return;

    auto myTransform = gameObject_->GetComponent<TransformComponent>();
    if (!myTransform) return;

    // プレイヤーの向き（回転角度）から進行方向をベースとしたローカル座標系を作成
    float yaw = targetTransform_->GetRotation().y;
    float pitch = targetTransform_->GetRotation().x;

    // プレイヤーを基準とした回転行列の方向成分を計算
    Vector3 forward = {
        std::sin(yaw) * std::cos(pitch),
        std::sin(-pitch),
        std::cos(yaw) * std::cos(pitch)
    };
    
    // 正規化
    float len = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    if (len > 0.0001f) {
        forward.x /= len; forward.y /= len; forward.z /= len;
    } else {
        forward = {0.0f, 0.0f, 1.0f};
    }

    // 右ベクトルと上ベクトルの算出 (外積)
    Vector3 upVec = {0.0f, 1.0f, 0.0f};
    Vector3 right = {
        upVec.y * forward.z - upVec.z * forward.y,
        upVec.z * forward.x - upVec.x * forward.z,
        upVec.x * forward.y - upVec.y * forward.x
    };
    float rLen = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
    if (rLen > 0.0001f) {
        right.x /= rLen; right.y /= rLen; right.z /= rLen;
    } else {
        right = {1.0f, 0.0f, 0.0f};
    }

    Vector3 up = {
        forward.y * right.z - forward.z * right.y,
        forward.z * right.x - forward.x * right.z,
        forward.x * right.y - forward.y * right.x
    };

    // プレイヤー位置に、プレイヤーの向きに基づいたローカルオフセットを足す
    Vector3 targetCamPos = {
        targetTransform_->GetPosition().x + right.x * offset_.x + up.x * offset_.y + forward.x * offset_.z,
        targetTransform_->GetPosition().y + right.y * offset_.x + up.y * offset_.y + forward.y * offset_.z,
        targetTransform_->GetPosition().z + right.z * offset_.x + up.z * offset_.y + forward.z * offset_.z
    };

    // 滑らかな追従 (線形補間/Lerp) を行う (フレームレート非依存)
    float t = 1.0f - std::pow(followDelay_, deltaTime); 
    Vector3 newPos = myTransform->GetPosition();
    newPos.x += (targetCamPos.x - newPos.x) * t;
    newPos.y += (targetCamPos.y - newPos.y) * t;
    newPos.z += (targetCamPos.z - newPos.z) * t;
    myTransform->SetPosition(newPos);

    // カメラの向き（角度）もプレイヤーの向きに追従させる（Lerpで滑らかに旋回）
    Vector3 newRot = myTransform->GetRotation();
    newRot.x += (targetTransform_->GetRotation().x - newRot.x) * t;
    newRot.y += (targetTransform_->GetRotation().y - newRot.y) * t;
    newRot.z += (targetTransform_->GetRotation().z - newRot.z) * t;
    myTransform->SetRotation(newRot);
}

nlohmann::json TargetFollowComponent::Serialize() {
    nlohmann::json j = Component::Serialize();
    j["targetName"] = targetName_;
    j["offset"] = {offset_.x, offset_.y, offset_.z};
    j["followDelay"] = followDelay_;
    return j;
}

void TargetFollowComponent::Deserialize(const nlohmann::json& j) {
    Component::Deserialize(j);
    if (j.contains("targetName")) {
        targetName_ = j["targetName"].get<std::string>();
    }
    if (j.contains("offset")) {
        offset_.x = j["offset"][0].get<float>();
        offset_.y = j["offset"][1].get<float>();
        offset_.z = j["offset"][2].get<float>();
    }
    if (j.contains("followDelay")) {
        followDelay_ = j["followDelay"].get<float>();
    }
}
