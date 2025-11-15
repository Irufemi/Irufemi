#include "ObjClass.h"
#include <filesystem>
#include <algorithm>
#include <Windows.h>
#include "source/Texture.h"
#include "function/Math.h"
#include "manager/TextureManager.h"
#include "manager/DrawManager.h"
#include "manager/DebugUI.h"
#include "manager/ModelManager.h"
#include <imgui.h>
#include "engine/directX/DirectXCommon.h"

// (静的メンバ定義は変更なし)
TextureManager* ObjClass::textureManager_ = nullptr;
DrawManager* ObjClass::drawManager_ = nullptr;
DebugUI* ObjClass::ui_ = nullptr;
ModelManager* ObjClass::modelManager_ = nullptr;

void ObjClass::Initialize(Camera* camera, const std::string& filename) {
    camera_ = camera;
    textures_.clear();
    instanceResources_.clear();

    assert(modelManager_ && "ObjClass::Initialize: ModelManager is not set.");
    managedModel_ = modelManager_->GetModel(filename);

    if (!managedModel_ || !managedModel_->cpuModel) {
        OutputDebugStringA("[ObjClass] Initialize: model load failed.\n");
        return;
    }

    const auto& cpuModel = managedModel_->cpuModel;
    textures_.reserve(cpuModel->meshes.size());
    instanceResources_.reserve(cpuModel->meshes.size());

    for (const auto& mesh : cpuModel->meshes) {
        auto res = std::make_unique<D3D12ResourceUtil>();

        // --- インスタンス固有リソースの生成 ---
        // マテリアル
        res->materialResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(Material));
        res->materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->materialData_));
        
        // ObjMaterial から Material へ必要なデータをコピー
        res->materialData_->color = mesh.material.color;
        res->materialData_->enableLighting = mesh.material.enableLighting;
        res->materialData_->uvTransform = mesh.material.uvTransform;
        res->materialData_->shininess = mesh.material.shininess;
        res->materialData_->hasTexture = !mesh.material.textureFilePath.empty();
        res->materialData_->lightingMode = mesh.material.enableLighting ? 2 : 0;

        if (res->materialData_->color.w <= 0.0f) { res->materialData_->color.w = 1.0f; }

        // 行列
        res->transformationResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(TransformationMatrix));
        res->transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->transformationData_));
        
        // ライト
        res->directionalLightResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(DirectionalLight));
        res->directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->directionalLightData_));
        res->directionalLightData_->color = { 1,1,1,1 };
        res->directionalLightData_->direction = { 0,-1,0 };
        res->directionalLightData_->intensity = 1.0f;

        // カメラ
        res->cameraResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(CameraForGPU));
        res->cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->cameraData_));
        res->cameraData_->worldPosition = camera_->GetTranslate();

        // テクスチャ（これはインスタンスごとではなく、本来はMaterial共有が望ましいが、今回は従来通り）
        auto tex = std::make_unique<Texture>();
        if (!mesh.material.textureFilePath.empty()) {
            // (パス解決処理は変更なし)
            // ...
            tex->Initialize(mesh.material.textureFilePath);
            res->textureHandle_ = tex->GetTextureSrvHandleGPU();
            res->materialData_->hasTexture = true;
        } else {
            res->materialData_->hasTexture = false;
            res->textureHandle_ = textureManager_->GetWhiteTextureHandle();
        }
        
        textures_.push_back(std::move(tex));
        instanceResources_.push_back(std::move(res));
    }
    // 初回Updateを呼んでおく
    Update();
}

void ObjClass::Update(const char* objName) {
    // (ImGui部分は instanceResources_ を参照するように変更)
#if defined(_DEBUG) || defined(DEVELOPMENT)
    std::string name = std::string("Obj: ") + objName;
    ImGui::Begin(name.c_str());
    for (size_t i = 0; i < instanceResources_.size(); ++i) {
        auto& res = instanceResources_[i];
        std::string meshLabel = "Mesh[" + std::to_string(i) + "]";
        if (ImGui::TreeNode(meshLabel.c_str())) {
            ui_->DebugTransform(res->transform_);
            ui_->DebugMaterialBy3D(res->materialData_);
            ui_->DebugDirectionalLight(res->directionalLightData_);
            ui_->DebugUvTransform(res->uvTransform_);
            ImGui::TreePop();
        }
    }
    ImGui::End();
#endif

    if (!managedModel_) return;

    for (size_t i = 0; i < instanceResources_.size(); ++i) {
        auto& r = instanceResources_[i];
        
        r->transformationMatrix_.world = Math::MakeAffineMatrix(r->transform_.scale, r->transform_.rotate, r->transform_.translate);
        Matrix4x4 worldViewProj = Math::Multiply(r->transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));
        
        // rootNode 行列を適用
        if (managedModel_->cpuModel) {
            worldViewProj = managedModel_->cpuModel->rootNode.localMatrix * worldViewProj;
            r->transformationMatrix_.world = managedModel_->cpuModel->rootNode.localMatrix * r->transformationMatrix_.world;
        }
        
        r->transformationMatrix_.WVP = worldViewProj;

        Matrix4x4 worldForNormal = r->transformationMatrix_.world;
        worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f;
        worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
        r->transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
        
        *r->transformationData_ = r->transformationMatrix_;

        r->materialData_->uvTransform = Math::MakeAffineMatrix(r->uvTransform_.scale, r->uvTransform_.rotate, r->uvTransform_.translate);
        r->directionalLightData_->direction = Math::Normalize(r->directionalLightData_->direction);
        r->cameraData_->worldPosition = camera_->GetTranslate();
    }
}

void ObjClass::Draw() {
    if (!managedModel_ || instanceResources_.empty()) return;

    for (size_t i = 0; i < managedModel_->gpuMeshes.size(); ++i) {
        auto& gpuMesh = managedModel_->gpuMeshes[i];
        auto& instanceRes = instanceResources_[i];

        // 共有リソースとインスタンス固有リソースをDrawManagerに渡す
        // (DrawManager側の修正が必要。ここでは仮の呼び出し)
        drawManager_->DrawSharedMesh(gpuMesh.get(), instanceRes.get());
    }
}
