#include "TransformComponent.h"
#include "../GameObject.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Core/System/ComponentPool.h"

// Setters
void TransformComponent::SetPosition(const Irufemi::Vector3& position) {
    if (position_.x != position.x || position_.y != position.y || position_.z != position.z) {
        position_ = position;
        MarkLocalDirty();
    }
}

void TransformComponent::SetRotation(const Irufemi::Vector3& rotation) {
    if (rotation_.x != rotation.x || rotation_.y != rotation.y || rotation_.z != rotation.z) {
        rotation_ = rotation;
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
            Irufemi::Matrix4x4 parentMat = parentT->GetWorldMatrix();
            if (!inheritScale_) {
                // スケールを除去
                Irufemi::Vector3 xaxis = Irufemi::Math::Normalize(Irufemi::Vector3{ parentMat.m[0][0], parentMat.m[0][1], parentMat.m[0][2] });
                Irufemi::Vector3 yaxis = Irufemi::Math::Normalize(Irufemi::Vector3{ parentMat.m[1][0], parentMat.m[1][1], parentMat.m[1][2] });
                Irufemi::Vector3 zaxis = Irufemi::Math::Normalize(Irufemi::Vector3{ parentMat.m[2][0], parentMat.m[2][1], parentMat.m[2][2] });
                parentMat.m[0][0] = xaxis.x; parentMat.m[0][1] = xaxis.y; parentMat.m[0][2] = xaxis.z;
                parentMat.m[1][0] = yaxis.x; parentMat.m[1][1] = yaxis.y; parentMat.m[1][2] = yaxis.z;
                parentMat.m[2][0] = zaxis.x; parentMat.m[2][1] = zaxis.y; parentMat.m[2][2] = zaxis.z;
            }
            return parentMat;
        }
    }
    return Irufemi::Math::MakeIdentity4x4();
}

// --- Setters (World) ---
void TransformComponent::SetWorldPosition(const Irufemi::Vector3& worldPosition) {
    if (auto parent = gameObject_->GetParent()) {
        if (auto parentT = parent->GetComponent<TransformComponent>()) {
            CheckAndComputeMatrix();
            Irufemi::Matrix4x4 invMat = Irufemi::Math::Inverse(GetParentMatrixForChild());
            position_ = Irufemi::Math::Transform(worldPosition, invMat);
            MarkLocalDirty();
            return;
        }
    }
    SetPosition(worldPosition);
}

void TransformComponent::SetWorldRotation(const Irufemi::Vector3& worldRotation) {
    if (auto parent = gameObject_->GetParent()) {
        if (auto parentT = parent->GetComponent<TransformComponent>()) {
            // 最新のワールド行列からスケールと位置を維持して、回転だけを差し替える
            CheckAndComputeMatrix();
            Irufemi::Vector3 wPos = { worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
            Irufemi::Vector3 xaxis = { worldMatrix_.m[0][0], worldMatrix_.m[0][1], worldMatrix_.m[0][2] };
            Irufemi::Vector3 yaxis = { worldMatrix_.m[1][0], worldMatrix_.m[1][1], worldMatrix_.m[1][2] };
            Irufemi::Vector3 zaxis = { worldMatrix_.m[2][0], worldMatrix_.m[2][1], worldMatrix_.m[2][2] };
            Irufemi::Vector3 wScale = { Irufemi::Math::Length(xaxis), Irufemi::Math::Length(yaxis), Irufemi::Math::Length(zaxis) };
            
            Irufemi::Matrix4x4 newWorldMat = Irufemi::Math::MakeAffineMatrix(wScale, worldRotation, wPos);
            Irufemi::Matrix4x4 invMat = Irufemi::Math::Inverse(GetParentMatrixForChild());
            Irufemi::Matrix4x4 localMat = Irufemi::Math::Multiply(newWorldMat, invMat);
            
            rotation_ = Irufemi::Math::ExtractEulerFromMatrix(localMat);
            MarkLocalDirty();
            return;
        }
    }
    SetRotation(worldRotation);
}

void TransformComponent::SetWorldScale(const Irufemi::Vector3& worldScale) {
    if (auto parent = gameObject_->GetParent()) {
        if (auto parentT = parent->GetComponent<TransformComponent>()) {
            CheckAndComputeMatrix();
            Irufemi::Vector3 wPos = { worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
            Irufemi::Vector3 wRot = Irufemi::Math::ExtractEulerFromMatrix(worldMatrix_);
            
            Irufemi::Matrix4x4 newWorldMat = Irufemi::Math::MakeAffineMatrix(worldScale, wRot, wPos);
            Irufemi::Matrix4x4 invMat = Irufemi::Math::Inverse(GetParentMatrixForChild());
            Irufemi::Matrix4x4 localMat = Irufemi::Math::Multiply(newWorldMat, invMat);
            
            Irufemi::Vector3 xaxis = { localMat.m[0][0], localMat.m[0][1], localMat.m[0][2] };
            Irufemi::Vector3 yaxis = { localMat.m[1][0], localMat.m[1][1], localMat.m[1][2] };
            Irufemi::Vector3 zaxis = { localMat.m[2][0], localMat.m[2][1], localMat.m[2][2] };
            scale_ = { Irufemi::Math::Length(xaxis), Irufemi::Math::Length(yaxis), Irufemi::Math::Length(zaxis) };
            
            MarkLocalDirty();
            return;
        }
    }
    SetScale(worldScale);
}

void TransformComponent::SetWorldMatrix(const Irufemi::Matrix4x4& worldMatrix) {
    Irufemi::Matrix4x4 localMat = worldMatrix;
    if (auto parent = gameObject_->GetParent()) {
        if (auto parentT = parent->GetComponent<TransformComponent>()) {
            CheckAndComputeMatrix(); // 親の行列を最新化するため
            Irufemi::Matrix4x4 invMat = Irufemi::Math::Inverse(GetParentMatrixForChild());
            localMat = Irufemi::Math::Multiply(worldMatrix, invMat);
        }
    }
    
    position_ = { localMat.m[3][0], localMat.m[3][1], localMat.m[3][2] };
    rotation_ = Irufemi::Math::ExtractEulerFromMatrix(localMat);
    
    // スケールを計算
    Irufemi::Vector3 xaxis = { localMat.m[0][0], localMat.m[0][1], localMat.m[0][2] };
    Irufemi::Vector3 yaxis = { localMat.m[1][0], localMat.m[1][1], localMat.m[1][2] };
    Irufemi::Vector3 zaxis = { localMat.m[2][0], localMat.m[2][1], localMat.m[2][2] };
    scale_ = { Irufemi::Math::Length(xaxis), Irufemi::Math::Length(yaxis), Irufemi::Math::Length(zaxis) };
    
    MarkLocalDirty();
}

void TransformComponent::UpdateMatrixImmediate() {
    MarkWorldDirty();
    ComputeMatrix(true);
}

// Lazy Evaluated World Getters
const Irufemi::Vector3& TransformComponent::GetWorldPosition() const {
    CheckAndComputeMatrix();
    if (!isWorldTransformExtracted_) {
        // ワールド行列から再抽出する
        worldRotation_ = Irufemi::Math::ExtractEulerFromMatrix(worldMatrix_);
        Irufemi::Vector3 xaxis = { worldMatrix_.m[0][0], worldMatrix_.m[0][1], worldMatrix_.m[0][2] };
        Irufemi::Vector3 yaxis = { worldMatrix_.m[1][0], worldMatrix_.m[1][1], worldMatrix_.m[1][2] };
        Irufemi::Vector3 zaxis = { worldMatrix_.m[2][0], worldMatrix_.m[2][1], worldMatrix_.m[2][2] };
        worldScale_ = { Irufemi::Math::Length(xaxis), Irufemi::Math::Length(yaxis), Irufemi::Math::Length(zaxis) };
        worldPosition_ = { worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
        
        isWorldTransformExtracted_ = true;
    }
    return worldPosition_;
}

const Irufemi::Vector3& TransformComponent::GetWorldRotation() const {
    CheckAndComputeMatrix();
    if (!isWorldTransformExtracted_) {
        GetWorldPosition(); // 共通化して再利用
    }
    return worldRotation_;
}

const Irufemi::Vector3& TransformComponent::GetWorldScale() const {
    CheckAndComputeMatrix();
    if (!isWorldTransformExtracted_) {
        GetWorldPosition(); // 共通化して再利用
    }
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
    if (currentParent) {
        if (auto parentT = currentParent->GetComponent<TransformComponent>()) {
            // 親がDirtyなら計算させる（再帰的）
            parentT->CheckAndComputeMatrix();
            
            uint64_t currentParentVersion = parentT->GetTransformVersion();
            if (parentTransformVersionLastComputed_ != currentParentVersion ||
                parentLastComputed_ != currentParent) {
                parentChanged = true;
                parentTransformVersionLastComputed_ = currentParentVersion;
                parentLastComputed_ = currentParent;
            }
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
        if (gameObject_->GetParent() && gameObject_->GetParent()->GetComponent<TransformComponent>()) {
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
    j["rotation"] = { rotation_.x, rotation_.y, rotation_.z };
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
        rotation_.x = j["rotation"][0];
        rotation_.y = j["rotation"][1];
        rotation_.z = j["rotation"][2];
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
