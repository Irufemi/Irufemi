#include "AnimationModel.h"

#include "Application/camera/Camera.h"
#include "manager/ModelManager.h"
#include "manager/AnimationManager.h"
#include "manager/DrawManager.h"
#include "manager/DebugUI.h"
#include "engine/directX/DirectXCommon.h"
#include "function/Math.h"
#include "math/Material.h"
#include "math/ObjModel.h"
#include <cmath>

// 静的メンバ定義
TextureManager* AnimationModel::textureManager_ = nullptr;
DrawManager* AnimationModel::drawManager_ = nullptr;
DebugUI* AnimationModel::ui_ = nullptr;
ModelManager* AnimationModel::modelManager_ = nullptr;
AnimationManager* AnimationModel::animationManager_ = nullptr;

// 初期化
void AnimationModel::Initialize(Camera* camera, const std::string& filename) {
    camera_ = camera;

    assert(modelManager_ && "AnimationModel::Initialize: ModelManager is not set.");
    // ModelManagerから共有モデルを取得するだけ
    managedModel_ = modelManager_->GetModel(filename);

    if (!managedModel_ || !managedModel_->cpuModel) {
        OutputDebugStringA("[AnimationModel] Initialize: model load failed.\n");
        return;
    }

    // 変換行列リソースの生成とマップ
    assert(drawManager_ && "DrawManager is not set. Cannot get DirectXCommon.");
    transformationResource_ = drawManager_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
    transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationData_));

    assert(animationManager_ && "AnimationModel::Initialize: AnimationManager is not set.");
    animation_ = animationManager_->LoadAnimationFile(filename);

    // 初回Updateを呼んでおく
    Update();

    animationTime_ = 0.0f;
}

// 更新
void AnimationModel::Update() {

    if (!managedModel_ || !camera_) return;

    worldMatrix_ = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    UpdateAnimation();

    // オブジェクト全体のワールド行列を計算
    transformationMatrix_.WVP = localMatrix_ * worldMatrix_ * (camera_->GetViewMatrix() * camera_->GetPerspectiveFovMatrix());
    transformationMatrix_.world = localMatrix_ * worldMatrix_;

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

// 描画
void AnimationModel::Draw() {

    if (!managedModel_ || !drawManager_) return;

    // モデルと、このオブジェクトが持つ変換行列リソースのGPUアドレスを渡して描画を依頼
    drawManager_->DrawModel(managedModel_.get(), GetTransformationGpuAddress());
}

// デバッグ
void AnimationModel::Debug([[maybe_unused]] const char* objName) {
#if defined USE_IMGUI
    std::string name = std::string("AnimationModel: ") + objName;
    ImGui::Begin(name.c_str());
    if (ui_) {
        ui_->DebugTransform(transform_);

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
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
    ImGui::End();
#endif
}

void AnimationModel::UpdateMaterials() {
    if (!managedModel_ || !managedModel_->cpuModel || managedModel_->gpuMaterials.empty()) {
        return;
    }

    // 全メッシュのマテリアルを更新
    for (size_t i = 0; i < managedModel_->cpuModel->meshes.size(); ++i) {
        // インデックスが範囲内か確認
        if (i >= managedModel_->gpuMaterials.size()) {
            continue;
        }

        const ObjMaterial& cpuMat = managedModel_->cpuModel->meshes[i].material;
        GpuMaterial* gpuMat = managedModel_->gpuMaterials[i].get();

        if (gpuMat && gpuMat->materialResource) {
            Material* mappedData = nullptr;
            gpuMat->materialResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
            if (mappedData) {
                mappedData->color = cpuMat.color;
                mappedData->enableLighting = cpuMat.enableLighting;
                mappedData->uvTransform = cpuMat.uvTransform;
                mappedData->shininess = cpuMat.shininess;
                mappedData->hasTexture = !cpuMat.textureFilePath.empty();
                // ライティングモードを enableLighting に基づいて設定
                mappedData->lightingMode = cpuMat.enableLighting ? 2 : 0;
                if (mappedData->color.w <= 0.0f) { mappedData->color.w = 1.0f; }

                gpuMat->materialResource->Unmap(0, nullptr);
            }
        }
    }
}


void AnimationModel::UpdateAnimation() {

    // アニメーションの処理

    animationTime_ += 1.0f / 60.0f; // 時刻を進める。1/60で固定してあるが、計測した時間を使って可変フレームを対応したほうが望ましい。
    animationTime_ = std::fmod(animationTime_, animation_.duration); // 最後まで言ったら最初からリピート再生。リピートしなくても別にいい。
    NodeAnimation& rootNodeAnimation = animation_.nodeAnimations[managedModel_->cpuModel->rootNode.name]; // rootNodeのAnimationを取得
    Vector3 translate = AnimationManager::CalculateValue(rootNodeAnimation.translate, animationTime_); /// 指定時刻の値を取得
    Quaternion rotate = AnimationManager::CalculateValue(rootNodeAnimation.rotate, animationTime_);
    Vector3 scale = AnimationManager::CalculateValue(rootNodeAnimation.scale, animationTime_);
    localMatrix_ = Math::MakeAffineMatrix(scale, rotate, translate);

}

const ObjMaterial* AnimationModel::GetMaterial(size_t meshIndex) const {
    if (managedModel_ && managedModel_->cpuModel && meshIndex < managedModel_->cpuModel->meshes.size()) {
        return &managedModel_->cpuModel->meshes[meshIndex].material;
    }
    return nullptr;
}

ObjMaterial* AnimationModel::GetMaterial(size_t meshIndex) {
    if (managedModel_ && managedModel_->cpuModel && meshIndex < managedModel_->cpuModel->meshes.size()) {
        return &managedModel_->cpuModel->meshes[meshIndex].material;
    }
    return nullptr;
}