#include "SkinnedMeshRendererComponent.h"
#include "../../GameObject.h"
#include "../TransformComponent.h"
#include "Engine/IrufemiEngine.h"

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
