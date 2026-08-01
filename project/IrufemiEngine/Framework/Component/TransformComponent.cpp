#include "TransformComponent.h"
#include "../GameObject.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Core/System/ComponentPool.h"
#include <cmath>

// Setters
void TransformComponent::SetPosition(const Irufemi::Vector3& position) {
    if (position_.x != position.x || position_.y != position.y || position_.z != position.z) {
        position_ = position;
        MarkLocalDirty();
    }
}

void TransformComponent::SetRotation(const Irufemi::Vector3& rotation) {
    SetRotationQuat(Irufemi::Math::ToQuaternionFromEuler(rotation));
}

void TransformComponent::SetRotationQuat(const Irufemi::Quaternion& rotation) {
    if (rotation_.x != rotation.x || rotation_.y != rotation.y || rotation_.z != rotation.z || rotation_.w != rotation.w) {
        rotation_ = Irufemi::Math::Normalize(rotation);
        MarkLocalDirty();
    }
}

void TransformComponent::SetScale(const Irufemi::Vector3& scale) {
    if (scale_.x != scale.x || scale_.y != scale.y || scale_.z != scale.z) {
        scale_ = scale;
        MarkLocalDirty();
    }
}

// --- GetParentMatrixForChild ---
Irufemi::Matrix4x4 TransformComponent::GetParentMatrixForChild() const {
    if (auto parent = gameObject_->GetParent()) {
        if (auto parentT = parent->GetComponent<TransformComponent>()) {
            if (!inheritScale_) {
                // スケールを除去し、回転と位置だけで再構築（せん断防止）
                // 親のGetWorldPosition()等を呼ぶと抽出処理が走るため、既に計算済みのWorldMatrixから直接抽出する
                const Irufemi::Matrix4x4& parentWorld = parentT->GetWorldMatrix();
                Irufemi::Vector3 pPos = { parentWorld.m[3][0], parentWorld.m[3][1], parentWorld.m[3][2] };
                Irufemi::Quaternion quat = Irufemi::Math::ToQuaternionFromMatrix(parentWorld);
                Irufemi::Vector3 pScale = { 1.0f, 1.0f, 1.0f };
                return Irufemi::Math::MakeAffineMatrix(pScale, quat, pPos);
            }
            return parentT->GetWorldMatrix();
        }
    }
    return Irufemi::Math::MakeIdentity4x4();
}

// --- Setters (World) ---
void TransformComponent::SetWorldPosition(const Irufemi::Vector3& worldPosition) {
    if (gameObject_) {
        if (auto parent = gameObject_->GetParent()) {
            if (auto parentT = parent->GetComponent<TransformComponent>()) {
                Irufemi::Matrix4x4 invMat = Irufemi::Math::Inverse(GetParentMatrixForChild());
                position_ = Irufemi::Math::Transform(worldPosition, invMat);
                MarkLocalDirty();
                return;
            }
        }
    }
    SetPosition(worldPosition);
}

void TransformComponent::SetWorldRotation(const Irufemi::Vector3& worldRotation) {
    SetWorldRotationQuat(Irufemi::Math::ToQuaternionFromEuler(worldRotation));
}

void TransformComponent::SetWorldRotationQuat(const Irufemi::Quaternion& worldRotation) {
    if (gameObject_) {
        if (auto parent = gameObject_->GetParent()) {
            if (auto parentT = parent->GetComponent<TransformComponent>()) {
                CheckAndComputeMatrix();
                Irufemi::Vector3 wPos = GetWorldPosition();
                Irufemi::Vector3 wScale = GetWorldScale();
                
                Irufemi::Matrix4x4 newWorldMat = Irufemi::Math::MakeAffineMatrix(wScale, worldRotation, wPos);
                Irufemi::Matrix4x4 invMat = Irufemi::Math::Inverse(GetParentMatrixForChild());
                Irufemi::Matrix4x4 localMat = Irufemi::Math::Multiply(newWorldMat, invMat);
                
                rotation_ = Irufemi::Math::Normalize(Irufemi::Math::ToQuaternionFromMatrix(localMat));
                MarkLocalDirty();
                return;
            }
        }
    }
    SetRotationQuat(worldRotation);
}

void TransformComponent::SetWorldScale(const Irufemi::Vector3& worldScale) {
    if (gameObject_) {
        if (auto parent = gameObject_->GetParent()) {
            if (auto parentT = parent->GetComponent<TransformComponent>()) {
                CheckAndComputeMatrix();
                Irufemi::Vector3 wPos = GetWorldPosition();
                Irufemi::Quaternion wRot = GetWorldRotationQuat();
                
                Irufemi::Matrix4x4 newWorldMat = Irufemi::Math::MakeAffineMatrix(worldScale, wRot, wPos);
                Irufemi::Matrix4x4 invMat = Irufemi::Math::Inverse(GetParentMatrixForChild());
                Irufemi::Matrix4x4 localMat = Irufemi::Math::Multiply(newWorldMat, invMat);
                
                Irufemi::Vector3 xaxis = { localMat.m[0][0], localMat.m[0][1], localMat.m[0][2] };
                Irufemi::Vector3 yaxis = { localMat.m[1][0], localMat.m[1][1], localMat.m[1][2] };
                Irufemi::Vector3 zaxis = { localMat.m[2][0], localMat.m[2][1], localMat.m[2][2] };
                
                // worldScale の符号（プラス/マイナス）を正確に維持する
                scale_ = { 
                    std::copysign(Irufemi::Math::Length(xaxis), worldScale.x), 
                    std::copysign(Irufemi::Math::Length(yaxis), worldScale.y), 
                    std::copysign(Irufemi::Math::Length(zaxis), worldScale.z) 
                };
                
                MarkLocalDirty();
                return;
            }
        }
    }
    SetScale(worldScale);
}

void TransformComponent::SetWorldMatrix(const Irufemi::Matrix4x4& worldMatrix) {
    Irufemi::Matrix4x4 localMat = worldMatrix;
    if (gameObject_) {
        if (auto parent = gameObject_->GetParent()) {
            if (auto parentT = parent->GetComponent<TransformComponent>()) {
                Irufemi::Matrix4x4 invMat = Irufemi::Math::Inverse(GetParentMatrixForChild());
                localMat = Irufemi::Math::Multiply(worldMatrix, invMat);
            }
        }
    }
    
    position_ = { localMat.m[3][0], localMat.m[3][1], localMat.m[3][2] };
    rotation_ = Irufemi::Math::Normalize(Irufemi::Math::ToQuaternionFromMatrix(localMat));
    
    // スケールを計算
    Irufemi::Vector3 xaxis = { localMat.m[0][0], localMat.m[0][1], localMat.m[0][2] };
    Irufemi::Vector3 yaxis = { localMat.m[1][0], localMat.m[1][1], localMat.m[1][2] };
    Irufemi::Vector3 zaxis = { localMat.m[2][0], localMat.m[2][1], localMat.m[2][2] };
    
    // 設定前のローカルスケールの符号を維持する
    scale_ = { 
        std::copysign(Irufemi::Math::Length(xaxis), scale_.x), 
        std::copysign(Irufemi::Math::Length(yaxis), scale_.y), 
        std::copysign(Irufemi::Math::Length(zaxis), scale_.z) 
    };
    
    MarkLocalDirty();
}

void TransformComponent::UpdateMatrixImmediate() {
    MarkWorldDirty();
    ComputeMatrix(true);
}

void TransformComponent::ExtractWorldTransform() const {
    if (isWorldTransformExtracted_) {
        return;
    }
    
    worldRotation_ = Irufemi::Math::Normalize(Irufemi::Math::ToQuaternionFromMatrix(worldMatrix_));
    Irufemi::Vector3 xaxis = { worldMatrix_.m[0][0], worldMatrix_.m[0][1], worldMatrix_.m[0][2] };
    Irufemi::Vector3 yaxis = { worldMatrix_.m[1][0], worldMatrix_.m[1][1], worldMatrix_.m[1][2] };
    Irufemi::Vector3 zaxis = { worldMatrix_.m[2][0], worldMatrix_.m[2][1], worldMatrix_.m[2][2] };
    
    // 親のワールドスケールの符号と自身のローカルスケールの符号から、現在のワールドスケールの符号を決定する
    Irufemi::Vector3 worldSign = scale_;
    if (gameObject_) {
        if (auto parent = gameObject_->GetParent()) {
            if (auto parentT = parent->GetComponent<TransformComponent>()) {
                if (inheritScale_) {
                    Irufemi::Vector3 pScale = parentT->GetWorldScale();
                    worldSign.x *= pScale.x;
                    worldSign.y *= pScale.y;
                    worldSign.z *= pScale.z;
                }
            }
        }
    }
    
    worldScale_ = { 
        std::copysign(Irufemi::Math::Length(xaxis), worldSign.x), 
        std::copysign(Irufemi::Math::Length(yaxis), worldSign.y), 
        std::copysign(Irufemi::Math::Length(zaxis), worldSign.z) 
    };
    worldPosition_ = { worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
    
    isWorldTransformExtracted_ = true;
}

// Lazy Evaluated World Getters
const Irufemi::Vector3& TransformComponent::GetWorldPosition() const {
    CheckAndComputeMatrix();
    ExtractWorldTransform();
    return worldPosition_;
}

Irufemi::Vector3 TransformComponent::GetWorldRotation() const {
    return Irufemi::Math::ToEuler(GetWorldRotationQuat());
}

const Irufemi::Quaternion& TransformComponent::GetWorldRotationQuat() const {
    CheckAndComputeMatrix();
    ExtractWorldTransform();
    return worldRotation_;
}

const Irufemi::Vector3& TransformComponent::GetWorldScale() const {
    CheckAndComputeMatrix();
    ExtractWorldTransform();
    return worldScale_;
}

Irufemi::Vector3 TransformComponent::GetWorldRight() const {
    CheckAndComputeMatrix();
    return Irufemi::Math::Normalize(Irufemi::Vector3{ worldMatrix_.m[0][0], worldMatrix_.m[0][1], worldMatrix_.m[0][2] });
}

Irufemi::Vector3 TransformComponent::GetWorldUp() const {
    CheckAndComputeMatrix();
    return Irufemi::Math::Normalize(Irufemi::Vector3{ worldMatrix_.m[1][0], worldMatrix_.m[1][1], worldMatrix_.m[1][2] });
}

Irufemi::Vector3 TransformComponent::GetWorldForward() const {
    CheckAndComputeMatrix();
    return Irufemi::Math::Normalize(Irufemi::Vector3{ worldMatrix_.m[2][0], worldMatrix_.m[2][1], worldMatrix_.m[2][2] });
}

void TransformComponent::ComputeMatrix(bool force) const {
    bool parentChanged = false;

    GameObject* currentParent = gameObject_->GetParent().get();
    TransformComponent* parentT = currentParent ? currentParent->GetComponent<TransformComponent>() : nullptr;

    if (parentT) {
        // 親がDirtyなら計算させる（再帰的）
        parentT->CheckAndComputeMatrix();
        
        uint64_t currentParentVersion = parentT->GetTransformVersion();
        if (parentTransformVersionLastComputed_ != currentParentVersion ||
            parentLastComputed_ != currentParent) {
            parentChanged = true;
            parentTransformVersionLastComputed_ = currentParentVersion;
            parentLastComputed_ = currentParent;
        }
    } else {
        if (parentLastComputed_ != nullptr) {
            parentChanged = true;
            parentLastComputed_ = nullptr;
            parentTransformVersionLastComputed_ = 0;
        }
    }

    // 強制更新でもなく、自分も親も変わっていないならスキップ (完全な遅延評価)
    if (!force && !isLocalDirty_ && !isWorldDirty_ && !parentChanged) {
        return;
    }

    // ローカル行列の計算（変更があった場合のみ）
    if (isLocalDirty_) {
        localMatrix_ = Irufemi::Math::MakeAffineMatrix(scale_, rotation_, position_);
        isLocalDirty_ = false;
        isWorldDirty_ = true; // ローカルが変わればワールドも必ず変わる
    }
    
    // ワールド行列の計算
    if (isWorldDirty_ || parentChanged || force) {
        if (parentT) {
            worldMatrix_ = Irufemi::Math::Multiply(localMatrix_, GetParentMatrixForChild());
        } else {
            worldMatrix_ = localMatrix_;
        }
        isWorldDirty_ = false;
        isWorldTransformExtracted_ = false; // ワールド座標が変化したので抽出フラグを下ろす
        transformVersion_++; // ワールド行列が更新されたので自身のバージョンを上げる
    }
}

void TransformComponent::UpdateAll() {
    ComponentPool<TransformComponent>::GetInstance().ForEach([](TransformComponent& transform) {
        transform.ComputeMatrix(false);
    });
}

nlohmann::json TransformComponent::Serialize() {
    nlohmann::json j;
    j["position"] = { position_.x, position_.y, position_.z };
    Irufemi::Vector3 euler = Irufemi::Math::ToEuler(rotation_);
    j["rotation"] = { euler.x, euler.y, euler.z };
    j["scale"]    = { scale_.x, scale_.y, scale_.z };
    j["inheritScale"] = inheritScale_;
    return j;
}

void TransformComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("position") && j["position"].is_array() && j["position"].size() == 3) {
        position_.x = j["position"][0];
        position_.y = j["position"][1];
        position_.z = j["position"][2];
    }
    if (j.contains("rotation") && j["rotation"].is_array() && j["rotation"].size() == 3) {
        Irufemi::Vector3 euler;
        euler.x = j["rotation"][0];
        euler.y = j["rotation"][1];
        euler.z = j["rotation"][2];
        rotation_ = Irufemi::Math::ToQuaternionFromEuler(euler);
    }
    if (j.contains("scale") && j["scale"].is_array() && j["scale"].size() == 3) {
        scale_.x = j["scale"][0];
        scale_.y = j["scale"][1];
        scale_.z = j["scale"][2];
    }
    if (j.contains("inheritScale") && j["inheritScale"].is_boolean()) {
        inheritScale_ = j["inheritScale"];
    }
    MarkLocalDirty(); // Deserialize時にDirtyにする
}
