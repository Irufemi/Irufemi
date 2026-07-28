#include "SkinnedMeshRendererComponent.h"
#include "../../GameObject.h"
#include "../TransformComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Math/Geometry/OBB.h"
#include <cmath>

SkinnedMeshRendererComponent::SkinnedMeshRendererComponent() {
    animatedMesh_ = std::make_unique<AnimatedMeshObject>();
}

SkinnedMeshRendererComponent::~SkinnedMeshRendererComponent() {}

void SkinnedMeshRendererComponent::Initialize() {
    if (!modelFilename_.empty()) {
        LoadModel(modelFilename_);
    }
}

void SkinnedMeshRendererComponent::LoadModel(const std::string& filename) {
    modelFilename_ = filename;
    currentLoadedFilename_ = filename;
    animatedMesh_->Initialize(modelFilename_);
}

void SkinnedMeshRendererComponent::Update() {
    // エディタ等で文字列が変更された場合の動的ロード検知
    if (modelFilename_ != currentLoadedFilename_) {
        LoadModel(modelFilename_);
    }

    if (auto transform = GetGameObject()->GetComponent<TransformComponent>()) {
        animatedMesh_->SetTranslate(transform->GetWorldPosition());
        animatedMesh_->SetRotate(transform->GetWorldRotation());
        animatedMesh_->SetScale(transform->GetWorldScale());
    }
    animatedMesh_->SetDebugBoneVisible(showDebugBones_);
    
    // Animatorからのポーズがあれば適用、なければバインドポーズ(nullptr)
    animatedMesh_->Update(poseOverride_);
    
    // 次フレームのためにリセット（毎フレーム指定される想定）
    poseOverride_ = nullptr;
}

void SkinnedMeshRendererComponent::Draw() {
    animatedMesh_->Draw();
}

bool SkinnedMeshRendererComponent::Raycast(const Ray& ray, float& outDistance) const {
    if (!animatedMesh_) return false;
    auto cpuModel = animatedMesh_->GetCpuModel();
    if (!cpuModel) return false;
    
    auto transform = GetGameObject()->GetComponent<TransformComponent>();
    if (!transform) return false;

    // ローカルAABBから中心とサイズを取得
    Vector3 localCenter = (cpuModel->boundingBox.min + cpuModel->boundingBox.max) * 0.5f;
    Vector3 localHalfSize = (cpuModel->boundingBox.max - cpuModel->boundingBox.min) * 0.5f;

    OBB obb;
    // ワールド行列を用いて中心点を変換
    const Matrix4x4& wmat = transform->GetWorldMatrix();
    obb.center = Math::Transform(localCenter, wmat);

    // ワールド行列の各軸ベクトルを抽出して正規化（回転）＆スケール適用
    Vector3 xAxis = { wmat.m[0][0], wmat.m[0][1], wmat.m[0][2] };
    Vector3 yAxis = { wmat.m[1][0], wmat.m[1][1], wmat.m[1][2] };
    Vector3 zAxis = { wmat.m[2][0], wmat.m[2][1], wmat.m[2][2] };

    float lenX = Math::Length(xAxis);
    float lenY = Math::Length(yAxis);
    float lenZ = Math::Length(zAxis);

    if (lenX > 0.0001f) obb.orientations[0] = Math::Normalize(xAxis);
    else obb.orientations[0] = {1.0f, 0.0f, 0.0f};

    if (lenY > 0.0001f) obb.orientations[1] = Math::Normalize(yAxis);
    else obb.orientations[1] = {0.0f, 1.0f, 0.0f};

    if (lenZ > 0.0001f) obb.orientations[2] = Math::Normalize(zAxis);
    else obb.orientations[2] = {0.0f, 0.0f, 1.0f};

    obb.size.x = localHalfSize.x * lenX;
    obb.size.y = localHalfSize.y * lenY;
    obb.size.z = localHalfSize.z * lenZ;

    return Collision::IsCollision(ray, obb, outDistance);
}

nlohmann::json SkinnedMeshRendererComponent::Serialize() {
    nlohmann::json j = nlohmann::json::object();
    j["Model File"] = modelFilename_;
    j["Show Debug Bones"] = showDebugBones_;
    return j;
}

void SkinnedMeshRendererComponent::OnRegisterProperties() {
    RegisterProperty("Model File", &modelFilename_)
        .SetTooltip("The path to the GLTF or OBJ file to load");
    RegisterProperty("Show Debug Bones", &showDebugBones_)
        .SetTooltip("Toggle bone visualization for this model");
}

void SkinnedMeshRendererComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("Model File")) modelFilename_ = j["Model File"].get<std::string>();
    if (j.contains("Show Debug Bones")) showDebugBones_ = j["Show Debug Bones"].get<bool>();
}
