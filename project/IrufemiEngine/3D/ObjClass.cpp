#include "ObjClass.h"
#include <filesystem>
#include <algorithm>
#include <Windows.h>
#include "function/Math.h"
#include "manager/TextureManager.h"
#include "manager/DrawManager.h"
#include "manager/DebugUI.h"
#include "manager/ModelManager.h"
#include <imgui.h>
#include "engine/directX/DirectXCommon.h"
#include "math/CameraForGPU.h" // 追加
#include "math/DirectionalLight.h" // 追加

// (静的メンバ定義は変更なし)
TextureManager* ObjClass::textureManager_ = nullptr;
DrawManager* ObjClass::drawManager_ = nullptr;
DebugUI* ObjClass::ui_ = nullptr;
ModelManager* ObjClass::modelManager_ = nullptr;

ObjClass::~ObjClass() {
    if (transformationResource_) {
        transformationResource_->Unmap(0, nullptr);
    }
}

void ObjClass::Initialize(Camera* camera, const std::string& filename) {
    camera_ = camera;

    assert(modelManager_ && "ObjClass::Initialize: ModelManager is not set.");
    // ModelManagerから共有モデルを取得するだけ
    managedModel_ = modelManager_->GetModel(filename);

    if (!managedModel_ || !managedModel_->cpuModel) {
        OutputDebugStringA("[ObjClass] Initialize: model load failed.\n");
        return;
    }

    // 変換行列リソースの生成とマップ
    assert(drawManager_ && "DrawManager is not set. Cannot get DirectXCommon.");
    transformationResource_ = drawManager_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
    transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationData_));


    // 初回Updateを呼んでおく
    Update();
}

void ObjClass::Update() {

    if (!managedModel_ || !camera_) return;

    // オブジェクト全体のワールド行列を計算
    transformationMatrix_.world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    
    // rootNodeの行列を適用（モデルデータに階層情報があれば）
    if (managedModel_->cpuModel) {
        transformationMatrix_.world = managedModel_->cpuModel->rootNode.localMatrix * transformationMatrix_.world;
    }

    Matrix4x4 worldViewProj = Math::Multiply(transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));
    transformationMatrix_.WVP = worldViewProj;

    // 法線変換用の逆転置行列
    Matrix4x4 worldForNormal = transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
    transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

    // 計算した行列をマップ済みのリソースにコピー
    if (transformationData_) {
        *transformationData_ = transformationMatrix_;
    }
}

void ObjClass::Draw() {
    if (!managedModel_ || !drawManager_) return;

    // モデルと、このオブジェクトが持つ変換行列リソースのGPUアドレスを渡して描画を依頼
    drawManager_->DrawModel(managedModel_.get(), GetTransformationGpuAddress());
}

void ObjClass::Debug([[maybe_unused]] const char* objName) {
#if defined USE_IMGUI
    std::string name = std::string("Obj: ") + objName;
    ImGui::Begin(name.c_str());
    if (ui_) {
        ui_->DebugTransform(transform_);
    }
    
    // ImGuiでマテリアルを編集
    if (managedModel_ && managedModel_->cpuModel) {
        for (size_t i = 0; i < managedModel_->cpuModel->meshes.size(); ++i) {
            std::string materialLabel = "Mesh " + std::to_string(i) + " Material";
            if (ImGui::TreeNode(materialLabel.c_str())) {
                ObjMaterial* mat = GetMaterial(i);
                if (mat) {
                    ImGui::ColorEdit4("Color", &mat->color.x);
                    ImGui::Checkbox("Enable Lighting", &mat->enableLighting);
                    ImGui::DragFloat("Shininess", &mat->shininess, 1.0f, 1.0f, 256.0f);
                }
                ImGui::TreePop();
            }
        }
    }

    ImGui::End();
#endif
}

size_t ObjClass::GetMeshCount() const {
    if (managedModel_ && managedModel_->cpuModel) {
        return managedModel_->cpuModel->meshes.size();
    }
    return 0;
}

const ObjMaterial* ObjClass::GetMaterial(size_t meshIndex) const {
    if (managedModel_ && managedModel_->cpuModel && meshIndex < managedModel_->cpuModel->meshes.size()) {
        return &managedModel_->cpuModel->meshes[meshIndex].material;
    }
    return nullptr;
}

ObjMaterial* ObjClass::GetMaterial(size_t meshIndex) {
    if (managedModel_ && managedModel_->cpuModel && meshIndex < managedModel_->cpuModel->meshes.size()) {
        return &managedModel_->cpuModel->meshes[meshIndex].material;
    }
    return nullptr;
}

void ObjClass::SetEnableLightingToAllMeshes(bool enable) {
    if (managedModel_ && managedModel_->cpuModel) {
        for (auto& mesh : managedModel_->cpuModel->meshes) {
            mesh.material.enableLighting = enable;
        }
    }
}