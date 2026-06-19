#include "TransformComponent.h"
#include "../GameObject.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Core/System/ComponentPool.h"

// Setters
void TransformComponent::SetPosition(const Vector3& position) {
    if (position_.x != position.x || position_.y != position.y || position_.z != position.z) {
        position_ = position;
        isLocalDirty_ = true;
    }
}

void TransformComponent::SetRotation(const Vector3& rotation) {
    if (rotation_.x != rotation.x || rotation_.y != rotation.y || rotation_.z != rotation.z) {
        rotation_ = rotation;
        isLocalDirty_ = true;
    }
}

void TransformComponent::SetScale(const Vector3& scale) {
    if (scale_.x != scale.x || scale_.y != scale.y || scale_.z != scale.z) {
        scale_ = scale;
        isLocalDirty_ = true;
    }
}

// Lazy Evaluated World Getters
const Vector3& TransformComponent::GetWorldPosition() const {
    if (!isWorldTransformExtracted_) {
        // ComputeMatrixは呼ばれている前提。ワールド行列から再抽出する
        worldPosition_ = { worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
    }
    return worldPosition_;
}

const Vector3& TransformComponent::GetWorldRotation() const {
    if (!isWorldTransformExtracted_) {
        worldRotation_ = Math::ExtractEulerFromMatrix(worldMatrix_);
        // worldScale_ も一緒に計算しておく
        Vector3 xaxis = { worldMatrix_.m[0][0], worldMatrix_.m[0][1], worldMatrix_.m[0][2] };
        Vector3 yaxis = { worldMatrix_.m[1][0], worldMatrix_.m[1][1], worldMatrix_.m[1][2] };
        Vector3 zaxis = { worldMatrix_.m[2][0], worldMatrix_.m[2][1], worldMatrix_.m[2][2] };
        worldScale_ = { Math::Length(xaxis), Math::Length(yaxis), Math::Length(zaxis) };
        
        worldPosition_ = { worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
        
        isWorldTransformExtracted_ = true;
    }
    return worldRotation_;
}

const Vector3& TransformComponent::GetWorldScale() const {
    if (!isWorldTransformExtracted_) {
        GetWorldRotation(); // worldRotation_ の計算と一緒に処理される
    }
    return worldScale_;
}

void TransformComponent::ComputeMatrix(bool force) {
    // 既に計算済みならスキップ
    if (!force && !isLocalDirty_ && !isWorldDirty_ && lastUpdateFrame_ == currentFrame_) {
        return;
    }

    bool localChanged = isLocalDirty_;

    // ローカル行列の計算（変更があった場合のみ）
    if (isLocalDirty_) {
        localMatrix_ = Math::MakeAffineMatrix(scale_, rotation_, position_);
        isLocalDirty_ = false;
        isWorldDirty_ = true; // ローカルが変わればワールドも必ず変わる
    }

    bool parentChanged = false;
    
    // 親のワールド行列を加味して自身のワールド行列を計算
    if (auto parent = gameObject_->GetParent()) {
        if (auto parentTransform = parent->GetComponent<TransformComponent>()) {
            // 親がDirtyなら親を先に計算（強制フラグは伝播しない）
            if (parentTransform->isLocalDirty_ || parentTransform->isWorldDirty_ || force) {
                parentTransform->ComputeMatrix(force);
            }
            
            // 自分か親のどちらかが更新されたならワールド行列を再計算
            if (isWorldDirty_ || parentTransform->lastUpdateFrame_ == currentFrame_) {
                worldMatrix_ = Math::Multiply(localMatrix_, parentTransform->GetWorldMatrix());
                isWorldDirty_ = false;
                isWorldTransformExtracted_ = false; // ワールド座標が変化したので抽出フラグを下ろす
                parentChanged = true;
            }
        } else if (isWorldDirty_) {
            worldMatrix_ = localMatrix_;
            isWorldDirty_ = false;
            isWorldTransformExtracted_ = false;
        }
    } else if (isWorldDirty_) {
        worldMatrix_ = localMatrix_;
        isWorldDirty_ = false;
        isWorldTransformExtracted_ = false;
    }

    lastUpdateFrame_ = currentFrame_;
}

void TransformComponent::UpdateAll() {
    currentFrame_++;
    ComponentPool<TransformComponent>::GetInstance().ForEach([](TransformComponent& transform) {
        transform.ComputeMatrix(false);
    });
}

nlohmann::json TransformComponent::Serialize() {
    nlohmann::json j;
    j["position"] = { position_.x, position_.y, position_.z };
    j["rotation"] = { rotation_.x, rotation_.y, rotation_.z };
    j["scale"]    = { scale_.x, scale_.y, scale_.z };
    return j;
}

void TransformComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("position") && j["position"].is_array() && j["position"].size() == 3) {
        position_.x = j["position"][0];
        position_.y = j["position"][1];
        position_.z = j["position"][2];
    }
    if (j.contains("rotation") && j["rotation"].is_array() && j["rotation"].size() == 3) {
        rotation_.x = j["rotation"][0];
        rotation_.y = j["rotation"][1];
        rotation_.z = j["rotation"][2];
    }
    if (j.contains("scale") && j["scale"].is_array() && j["scale"].size() == 3) {
        scale_.x = j["scale"][0];
        scale_.y = j["scale"][1];
        scale_.z = j["scale"][2];
    }
    isLocalDirty_ = true; // Deserialize時にDirtyにする
}
