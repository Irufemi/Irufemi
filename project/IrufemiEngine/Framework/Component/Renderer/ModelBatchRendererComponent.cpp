#include "ModelBatchRendererComponent.h"
#include "../../GameObject.h"
#include "../TransformComponent.h"
#include "Renderer/Object/Batch/ModelBatch.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Math/Geometry/OBB.h"
#include <cmath>

ModelBatchRendererComponent::ModelBatchRendererComponent() {}
ModelBatchRendererComponent::~ModelBatchRendererComponent() {}

void ModelBatchRendererComponent::LoadModel(const std::string& filename) {
    modelName_ = filename;
    if (batch_ && !modelName_.empty()) {
        batch_->Initialize(modelName_);
    }
}

void ModelBatchRendererComponent::Initialize() {
    batch_ = std::make_unique<ModelBatch>();
    if (!modelName_.empty()) {
        batch_->Initialize(modelName_);
    }
    
    // キャッシュされたGPUカリング設定を反映
    batch_->SetUseGPUCulling(useGPUCulling_);

    if (gameObject_) {
        transform_ = gameObject_->GetComponent<TransformComponent>();
    }
}

void ModelBatchRendererComponent::Update() {
    // ModelBatchは個別ではなく、複数描画を管理するため
    // Transform更新はここでは行わない、または親の座標自体も一つのInstanceとして扱うかになります。
    // 基本的には外部から AddInstance() を呼ばれることを想定するため、ここでは何もしません。
}

void ModelBatchRendererComponent::Draw() {
    if (batch_) {
        batch_->Draw();
    }
}

IRenderable* ModelBatchRendererComponent::GetRenderable() {
    return reinterpret_cast<IRenderable*>(batch_.get());
}

Sphere ModelBatchRendererComponent::GetWorldSphere() const {
    Sphere result = { Vector3{0,0,0}, 1.0f }; // default
    if (transform_) {
        result.center = transform_->worldPosition_;
        float maxScale = std::fmax(transform_->worldScale_.x, std::fmax(transform_->worldScale_.y, transform_->worldScale_.z));
        result.radius = maxScale;
    }
    return result;
}

bool ModelBatchRendererComponent::Raycast(const Ray& ray, float& outDistance) const {
    if (!batch_ || !transform_) return false;

    // バッチ全体のAABBや個々のインスタンスとのRaycastは重いため、
    // エディタ等での簡易選択用として親のTransformにのみ当たり判定を付ける
    Vector3 localHalfSize = {0.5f, 0.5f, 0.5f};

    OBB obb;
    const Matrix4x4& wmat = transform_->GetWorldMatrix();
    obb.center = transform_->worldPosition_;

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

nlohmann::json ModelBatchRendererComponent::Serialize() {
    nlohmann::json j;
    j["modelName"] = modelName_;
    return j;
}

void ModelBatchRendererComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("modelName")) {
        std::string modelName = j["modelName"];
        LoadModel(modelName);
    }
}

void ModelBatchRendererComponent::AddInstance(const Transform& t) {
    if (batch_) {
        batch_->AddInstance(t);
    }
}

void ModelBatchRendererComponent::AddInstanceWorld(const Matrix4x4& world) {
    if (batch_) {
        batch_->AddInstanceWorld(world);
    }
}

void ModelBatchRendererComponent::ClearInstances() {
    if (batch_) {
        batch_->ClearInstances();
    }
}

void ModelBatchRendererComponent::SetUseGPUCulling(bool use) {
    useGPUCulling_ = use;
    if (batch_) {
        batch_->SetUseGPUCulling(use);
    }
}
