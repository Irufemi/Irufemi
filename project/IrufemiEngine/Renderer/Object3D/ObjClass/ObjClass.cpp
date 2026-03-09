#include "ObjClass.h"
#include <filesystem>
#include <algorithm>
#include <Windows.h>
#include "Engine/Core/Math/Geometry/Math.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Resource/Model/ModelManager.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"

// 静的メンバ定義
TextureManager* ObjClass::textureManager_ = nullptr;
DrawManager* ObjClass::drawManager_ = nullptr;
DebugUI* ObjClass::ui_ = nullptr;
ModelManager* ObjClass::modelManager_ = nullptr;

ObjClass::~ObjClass() {
    if (transformationResource_) {
        transformationResource_->Unmap(0, nullptr);
    }
    for (size_t i = 0; i < instanceMaterials_.size(); ++i) {
        if (instanceMaterials_[i] && instanceMaterials_[i]->materialResource) {
            instanceMaterials_[i]->materialResource->Unmap(0, nullptr);
        }
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

    // インスタンス固有のマテリアルリソースを生成
    instanceMaterials_.resize(managedModel_->gpuMaterials.size());
    mappedMaterials_.resize(managedModel_->gpuMaterials.size());
    for (size_t i = 0; i < managedModel_->gpuMaterials.size(); ++i) {
        instanceMaterials_[i] = std::make_shared<GpuMaterial>();
        instanceMaterials_[i]->materialResource = drawManager_->GetDxCommon()->CreateBufferResource(sizeof(Material));
        instanceMaterials_[i]->materialResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedMaterials_[i]));
        // 共有のテクスチャハンドルをコピー
        instanceMaterials_[i]->textureHandle = managedModel_->gpuMaterials[i]->textureHandle;
    }


    // 初回Updateを呼んでおく
    Update();
}

void ObjClass::Update() {

    if (!managedModel_ || !camera_) return;

    // オブジェクト全体のワールド行列を計算
    transformationMatrix_.world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    
    // rootNodeの行列を適用(モデルデータに階層情報があれば)
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

    // マテリアル情報をGPUへ転送
    UpdateMaterials();
}

void ObjClass::Draw() {
    if (!managedModel_ || !drawManager_) return;

    // モデルと、このオブジェクトが持つ変換行列リソースのGPUアドレスを渡して描画を依頼
    drawManager_->DrawModel(managedModel_.get(), GetTransformationGpuAddress(), instanceMaterials_);
}

void ObjClass::Debug([[maybe_unused]] const char* objName) {
#if defined USE_IMGUI
    std::string name = std::string("Obj: ") + objName;
    ImGui::Begin(name.c_str());
    if (ui_) {
        ui_->DebugTransform(transform_);
        ImGui::ColorEdit4("Color", &color_.x); // インスタンスカラーを編集

        // ImGuiでマテリアルを編集
        if (managedModel_ && managedModel_->cpuModel) {
            for (size_t i = 0; i < managedModel_->cpuModel->meshes.size(); ++i) {
                std::string materialLabel = "Mesh " + std::to_string(i) + " Material";
                if (ImGui::TreeNode(materialLabel.c_str())) {
                    ObjMaterial* mat = GetMaterial(i);
                    if (mat) {
                        // unique_id を渡してコントロールIDの衝突を避ける
                        std::string unique_id = "##" + std::to_string(i);
                        ui_->DebugObjMaterial(mat, unique_id.c_str());

                        // テクスチャ選択
                        // 注意：この部分はObjClassがテクスチャのインデックスを保持する仕組みがないと完全には機能しません。
                        // 今はUIのみ表示します。
                        int tempIndex = 0; // ダミー
                        // ui_->DebugTexture(...)
                    }
                    ImGui::TreePop();
                }
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

void ObjClass::SetAlpha(float alpha) {
    color_.w = alpha;
}

void ObjClass::SetColor(const Vector4& color) {
    color_ = color;
}

void ObjClass::UpdateMaterials() {
    if (!managedModel_ || !managedModel_->cpuModel || mappedMaterials_.empty()) {
        return;
    }

    // 全メッシュのマテリアルを更新
    for (size_t i = 0; i < managedModel_->cpuModel->meshes.size(); ++i) {
        // インデックスが範囲内か確認
        if (i >= mappedMaterials_.size() || !mappedMaterials_[i]) {
            continue;
        }

        const ObjMaterial& cpuMat = managedModel_->cpuModel->meshes[i].material;
        Material* mappedData = mappedMaterials_[i];

        // インスタンスカラーとマテリアルカラーを乗算
        mappedData->color.x = cpuMat.color.x * color_.x;
        mappedData->color.y = cpuMat.color.y * color_.y;
        mappedData->color.z = cpuMat.color.z * color_.z;
        mappedData->color.w = cpuMat.color.w * color_.w;

        mappedData->enableLighting = cpuMat.enableLighting;
        mappedData->uvTransform = cpuMat.uvTransform;
        mappedData->shininess = cpuMat.shininess;
        mappedData->hasTexture = !cpuMat.textureFilePath.empty();
        mappedData->lightingMode = cpuMat.enableLighting ? 2 : 0;
        if (mappedData->color.w <= 0.0f) { mappedData->color.w = 1.0f; }
    }
}