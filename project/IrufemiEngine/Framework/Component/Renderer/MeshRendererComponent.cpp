#include "MeshRendererComponent.h"

#include "../../GameObject.h"
#include "../TransformComponent.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"

MeshRendererComponent::MeshRendererComponent() {}
MeshRendererComponent::~MeshRendererComponent() {}

void MeshRendererComponent::LoadModel(const std::string& filename) {
    modelName_ = filename;
    if (obj_) {
        obj_->Initialize(modelName_);
    }
}

void MeshRendererComponent::Initialize() {
    obj_ = std::make_unique<ObjClass>();
    obj_->Initialize(modelName_);

    // 親の GameObject から TransformComponent を探して保持しておく
    if (gameObject_) {
        transform_ = gameObject_->GetComponent<TransformComponent>();
    }
}

void MeshRendererComponent::Update() {
    // TransformComponent があれば、その座標を ObjClass に渡す（同期）
    if (transform_ && obj_) {
        obj_->SetTranslate(transform_->worldPosition_);
        obj_->SetRotate(transform_->worldRotation_);
        obj_->SetScale(transform_->worldScale_);
    }

    // ObjClass の行列計算などを実行
    if (obj_) {
        obj_->Update();
    }
}

void MeshRendererComponent::Draw() {
    // RenderGraph に向けて描画パケットを積む
    if (obj_) {
        obj_->Draw();
    }
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
