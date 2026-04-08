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
}

void ObjClass::Initialize(Camera* camera, const std::string& filename) {
    camera_ = camera;

    assert(modelManager_ && "ObjClass::Initialize: ModelManager is not set.");
    managedModel_ = modelManager_->GetModel(filename);
    auto status = managedModel_->status.load();

    if (status != ManagedModel::LoadingStatus::Loaded || !managedModel_->cpuModel) {
        return;
    }

    // 変換行列リソースの生成とマップ (全メッシュ共有用)
    assert(drawManager_ && "DrawManager is not set. Cannot get DirectXCommon.");
    transformationResource_ = drawManager_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
    transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationData_));

    // インスタンス固有の各メッシュ用リソースを生成
    meshResources_.clear();
    for (size_t i = 0; i < managedModel_->gpuMeshes.size(); ++i) {
        auto res = std::make_unique<Object3DResource>();
        
        // 外部の変換行列リソースを借用
        res->SetExternalTransformationResource(transformationResource_, transformationData_);
        
        // メッシュ固有の View を設定
        const auto& gpuMesh = managedModel_->gpuMeshes[i];
        res->vertexBufferView_ = gpuMesh->vertexBufferView;
        res->indexBufferView_ = gpuMesh->indexBufferView;
        res->indexCount_ = gpuMesh->indexCount;
        
        // マテリアルリソース等の生成
        res->CreateResource();
        res->Map();
        
        // 初期テクスチャハンドルを共有データからコピー
        const auto& gpuMaterial = (i < managedModel_->gpuMaterials.size()) ? managedModel_->gpuMaterials[i] : nullptr;
        if (gpuMaterial) {
            res->textureHandle_ = gpuMaterial->textureHandle;
        }

        meshResources_.push_back(std::move(res));
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

    // フラグ更新
    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
}

#include "../../../Engine/Core/Math/Geometry/Collision.h"
#include "../../../Engine/Core/Shape/Sphere.h"

void ObjClass::Draw() {
    if (!managedModel_ || !drawManager_ || !camera_) {
        return;
    }

    // カメラの行列が変更されたか、オブジェクト自体が変更されたかチェック
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &camera_->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &camera_->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirty_ || cameraChanged) {
        Update();
    }

    // 視錐台カリング
    if (isCullingEnabled_ && managedModel_->cpuModel) {
        const Sphere& modelSphere = managedModel_->cpuModel->boundingSphere;

        // ワールド空間の境界球を計算
        Sphere worldSphere;
        worldSphere.center = Math::Transform(modelSphere.center, transformationMatrix_.world);

        // スケールの最大値を適用して半径を変換
        float maxScale = (std::max)({ transform_.scale.x, transform_.scale.y, transform_.scale.z });
        worldSphere.radius = modelSphere.radius * maxScale * 1.1f; // 10% マージン

        // 判定
        if (!Collision::IsCollision(camera_->GetFrustum(), worldSphere)) {
            return; // 描画スキップ
        }
    }

    // モデル内の全メッシュを描画
    for (auto& res : meshResources_) {
        drawManager_->DrawStandard3D(res.get());
    }
}

void ObjClass::Debug([[maybe_unused]] const char* objName) {
#if defined USE_IMGUI
    std::string name = std::string("Obj: ") + objName;
    ImGui::Begin(name.c_str());
    DebugTab();
    ImGui::End();
#endif
}

void ObjClass::DebugTab() {
#if defined USE_IMGUI
    if (ui_) {
        ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);
        ui_->DebugTransform(transform_);
        ImGui::ColorEdit4("Color", &color_.x); // インスタンスカラーを編集
        ui_->DebugMaterialOverrides(&environmentCoefficient_, &lightingModeOverride_, &useClampSamplerOverride_, &enableLightingOverride_, "##OcOverrides");

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
    enableLightingOverride_ = enable ? 1 : 0;
    isDirty_ = true;
}

void ObjClass::SetAlpha(float alpha) {
    color_.w = alpha;
}

void ObjClass::SetColor(const Vector4& color) {
    color_ = color;
}

void ObjClass::UpdateMaterials() {
    if (!managedModel_ || !managedModel_->cpuModel || meshResources_.empty()) {
        return;
    }

    // 全メッシュのマテリアルを更新
    for (size_t i = 0; i < managedModel_->cpuModel->meshes.size(); ++i) {
        if (i >= meshResources_.size()) break;

        auto& res = meshResources_[i];
        if (!res->materialData_) continue;

        const ObjMaterial& cpuMat = managedModel_->cpuModel->meshes[i].material;
        Material* mappedData = res->materialData_;

        // インスタンスカラーとマテリアルカラーを乗算
        mappedData->color.x = cpuMat.color.x * color_.x;
        mappedData->color.y = cpuMat.color.y * color_.y;
        mappedData->color.z = cpuMat.color.z * color_.z;
        mappedData->color.w = cpuMat.color.w * color_.w;
        if (mappedData->color.w <= 0.0f) { mappedData->color.w = 1.0f; }

        // ライティングの有効状態 (個別上書き優先)
        int32_t finalEnableLighting = (enableLightingOverride_ != -1) ? (enableLightingOverride_ == 1) : (cpuMat.enableLighting ? 1 : 0);
        mappedData->enableLighting = finalEnableLighting;

        mappedData->uvTransform = cpuMat.uvTransform;
        mappedData->metallic = cpuMat.metallic;
        mappedData->roughness = cpuMat.roughness;
        mappedData->hasTexture = !cpuMat.textureFilePath.empty();

        // 映り込み係数 (モデル値 * インスタンス係数)
        mappedData->environmentCoefficient = cpuMat.environmentCoefficient * environmentCoefficient_;

        // ライティングモード (個別上書き優先、指定なしならモデル値、ライティング無効なら0)
        if (lightingModeOverride_ != -1) {
            mappedData->lightingMode = lightingModeOverride_;
        } else {
            mappedData->lightingMode = finalEnableLighting ? cpuMat.lightingMode : 0;
        }

        // サンプラー設定 (個別上書き優先)
        mappedData->useClampSampler = (useClampSamplerOverride_ != -1) ? useClampSamplerOverride_ : cpuMat.useClampSampler;
    }
}