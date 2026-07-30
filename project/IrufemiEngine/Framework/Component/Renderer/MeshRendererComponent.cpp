#include "MeshRendererComponent.h"

#include "../../GameObject.h"
#include "../TransformComponent.h"
#include "Renderer/Object/3D/StaticModelObject/StaticModelObject.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Math/Geometry/OBB.h"
#include <cmath>

MeshRendererComponent::MeshRendererComponent() {}
MeshRendererComponent::~MeshRendererComponent() {}

void MeshRendererComponent::LoadModel(const std::string& filename) {
    modelName_ = filename;
    if (obj_) {
        obj_->Initialize(modelName_);
    }
}

void MeshRendererComponent::Initialize() {
    if (!obj_) {
        obj_ = std::make_unique<StaticModelObject>();
        obj_->Initialize(modelName_);
    }

    // 親の GameObject から TransformComponent を探して保持しておく
    if (gameObject_) {
        transform_ = gameObject_->GetComponent<TransformComponent>();
    }
}

void MeshRendererComponent::SetEnableEffectMask(bool enable) {
    if (obj_) {
        obj_->SetEnableEffectMask(enable);
    }
}

void MeshRendererComponent::SetCustomEffectType(int32_t type) {
    if (obj_) {
        obj_->SetCustomEffectType(type);
    }
}

void MeshRendererComponent::SetCustomEffectParam(float param) {
    if (obj_) {
        obj_->SetCustomEffectParam(param);
    }
}


void MeshRendererComponent::Update() {
    // TransformComponent があれば、その座標を StaticModelObject に渡す（同期）
    if (transform_ && obj_) {
        obj_->SetTranslate(transform_->GetWorldPosition());
        obj_->SetRotate(transform_->GetWorldRotation());
        obj_->SetScale(transform_->GetWorldScale());
    }

    // StaticModelObject の行列計算などを実行
    if (obj_) {
        obj_->Update();
    }
}

void MeshRendererComponent::Draw() {
    if (!isVisible_ || !gameObject_ || !gameObject_->GetIsActive()) return;
    // RenderGraph に向けて描画パケットを積む
    if (obj_) {
        obj_->Draw();
    }
}

Irufemi::Sphere MeshRendererComponent::GetWorldSphere() const {
    Irufemi::Sphere result = { Irufemi::Vector3{0,0,0}, 1.0f }; // default
    if (transform_) {
        result.center = transform_->GetWorldPosition();
        // StaticModelObject の cpuModel があれば正確な半径を取得
        // ここでは便宜上スケールの最大値を半径として扱う（もしくは定数）
        Irufemi::Vector3 worldScale = transform_->GetWorldScale();
        float maxScale = std::fmax(worldScale.x, std::fmax(worldScale.y, worldScale.z));
        result.radius = maxScale;
    }
    return result;
}

bool MeshRendererComponent::Raycast(const Irufemi::Ray& ray, float& outDistance) const {
    if (!obj_ || !transform_) return false;
    auto cpuModel = obj_->GetCpuModel();
    if (!cpuModel) return false;

    // ローカルAABBから中心とサイズを取得
    Irufemi::Vector3 localCenter = (cpuModel->boundingBox.min + cpuModel->boundingBox.max) * 0.5f;
    Irufemi::Vector3 localHalfSize = (cpuModel->boundingBox.max - cpuModel->boundingBox.min) * 0.5f;

    Irufemi::OBB obb;
    // ワールド行列を用いて中心点を変換
    const Irufemi::Matrix4x4& wmat = transform_->GetWorldMatrix();
    obb.center = Irufemi::Math::Transform(localCenter, wmat);

    // ワールド行列の各軸ベクトルを抽出して正規化（回転）＆スケール適用
    Irufemi::Vector3 xAxis = { wmat.m[0][0], wmat.m[0][1], wmat.m[0][2] };
    Irufemi::Vector3 yAxis = { wmat.m[1][0], wmat.m[1][1], wmat.m[1][2] };
    Irufemi::Vector3 zAxis = { wmat.m[2][0], wmat.m[2][1], wmat.m[2][2] };

    float lenX = Irufemi::Math::Length(xAxis);
    float lenY = Irufemi::Math::Length(yAxis);
    float lenZ = Irufemi::Math::Length(zAxis);

    if (lenX > 0.0001f) obb.orientations[0] = Irufemi::Math::Normalize(xAxis);
    else obb.orientations[0] = {1.0f, 0.0f, 0.0f};

    if (lenY > 0.0001f) obb.orientations[1] = Irufemi::Math::Normalize(yAxis);
    else obb.orientations[1] = {0.0f, 1.0f, 0.0f};

    if (lenZ > 0.0001f) obb.orientations[2] = Irufemi::Math::Normalize(zAxis);
    else obb.orientations[2] = {0.0f, 0.0f, 1.0f};

    obb.size.x = localHalfSize.x * lenX;
    obb.size.y = localHalfSize.y * lenY;
    obb.size.z = localHalfSize.z * lenZ;

    return Irufemi::Collision::IsCollision(ray, obb, outDistance);
}




nlohmann::json MeshRendererComponent::Serialize() {
    nlohmann::json j;
    j["modelName"] = modelName_;
    return j;
}

void MeshRendererComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("modelName")) {
        std::string modelName = j["modelName"];
        LoadModel(modelName);
    }
}
