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
        animatedMesh_->Initialize(modelFilename_);
    }
}

void SkinnedMeshRendererComponent::LoadModel(const std::string& filename) {
    modelFilename_ = filename;
    animatedMesh_->Initialize(modelFilename_);
}

void SkinnedMeshRendererComponent::Update() {
    if (auto transform = GetGameObject()->GetComponent<TransformComponent>()) {
        animatedMesh_->SetTranslate(transform->GetWorldPosition());
        animatedMesh_->SetRotate(transform->GetWorldRotation());
        animatedMesh_->SetScale(transform->GetWorldScale());
    }
    // Update自体はAnimatorComponent側からポーズ付きで呼ばれることを想定し、
    // ここでは自身単体での更新（バインドポーズ）を行う
    animatedMesh_->Update(nullptr);
}

void SkinnedMeshRendererComponent::Draw() {
    animatedMesh_->Draw();
}

void SkinnedMeshRendererComponent::OnRegisterProperties() {
    RegisterProperty("Model File", &modelFilename_)
        .SetTooltip("The path to the GLTF or OBJ file to load");
}
