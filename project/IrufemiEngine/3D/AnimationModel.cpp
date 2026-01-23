#include "AnimationModel.h"

#include "Application/camera/Camera.h"
#include "manager/ModelManager.h"
#include "manager/AnimationManager.h"
#include "engine/directX/DirectXCommon.h"
#include "manager/DrawManager.h"
#include "manager/DebugUI.h"
#include "engine/IrufemiEngine.h"
#include "function/Math.h"
#include "math/Material.h"
#include "math/ObjModel.h"
#include "3D/SphereRegion.h"
#include "3D/LineClass.h"
#include <cmath>

// 静的メンバ定義
IrufemiEngine* AnimationModel::engine_ = nullptr;

AnimationModel::AnimationModel() {}
AnimationModel::~AnimationModel() {}

// 初期化
void AnimationModel::Initialize(Camera* camera, const std::string& filename) {
    camera_ = camera;

    assert(engine_ && "AnimationModel::Initialize: ModelManager is not set.");
    // ModelManagerから共有モデルを取得するだけ
    managedModel_ = engine_->GetObjModelManager()->GetModel(filename);

    if (!managedModel_ || !managedModel_->cpuModel) {
        OutputDebugStringA("[AnimationModel] Initialize: model load failed.\n");
        return;
    }

    // 変換行列リソースの生成とマップ
    assert(engine_ && "DrawManager is not set. Cannot get DirectXCommon.");
    transformationResource_ = engine_->GetDrawManager()->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
    transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationData_));

    // ノードアニメーション用のリソース確保
    if (managedModel_->cpuModel->skinClusterData.empty()) {
        meshTransformationResources_.resize(managedModel_->cpuModel->meshes.size());
        meshTransformationData_.resize(managedModel_->cpuModel->meshes.size());
        for (size_t i = 0; i < managedModel_->cpuModel->meshes.size(); ++i) {
            meshTransformationResources_[i] = engine_->GetDrawManager()->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
            meshTransformationResources_[i]->Map(0, nullptr, reinterpret_cast<void**>(&meshTransformationData_[i]));
        }
    }

    assert(engine_ && "AnimationModel::Initialize: AnimationManager is not set.");
    animation_ = engine_->GetAnimationManager()->LoadAnimationFile(filename);

    // Skeletonの生成
    if (managedModel_ && managedModel_->cpuModel) {
        // ModelManager(またはAnimationManager)にあるNode構造からSkeletonを作成
        skeleton_ = AnimationManager::CreateSkeleton(managedModel_->cpuModel->rootNode);

        // SkinClusterの生成 (スキニングモデルの場合のみ)
        if (!managedModel_->cpuModel->skinClusterData.empty()) {
            skinCluster_ = engine_->GetAnimationManager()->CreateSkinCluster(skeleton_, *managedModel_->cpuModel);
        }

        // 2. SphereRegionの初期化
        jointSpheres_ = std::make_unique<SphereRegion>();
        jointSpheres_->Initialize(camera, "resources/whiteTexture.png", 16);

        // 3. Jointの数だけ描画用インスタンスを「最初だけ」追加
        for (size_t i = 0; i < skeleton_.joints.size(); ++i) {
            Transform tf{};
            tf.scale = { 0.01f, 0.01f, 0.01f }; // 関節の大きさ
            jointSpheres_->AddInstance(tf);
        }

        // ボーン用のLine3DRegionを初期化
        boneLines_ = std::make_unique<Line3DRegion>();
        boneLines_->Initialize(camera);
    }

    // 初回Updateを呼んでおく
    Update();

    animationTime_ = 0.0f;
}

// 更新
void AnimationModel::Update() {

    if (!managedModel_ || !camera_) return;

    worldMatrix_ = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    UpdateAnimation();

    // スキニングモデルかノードアニメーションモデルかで処理を分岐
    if (!managedModel_->cpuModel->skinClusterData.empty()) {
        // --- スキニングモデルの更新 ---
        transformationMatrix_.WVP = worldMatrix_ * (camera_->GetViewMatrix() * camera_->GetPerspectiveFovMatrix());
        transformationMatrix_.world = worldMatrix_;
        Matrix4x4 worldForNormal = transformationMatrix_.world;
        worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f;
        worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
        transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
        if (transformationData_) {
            *transformationData_ = transformationMatrix_;
        }
    } else {
        // --- ノードアニメーションモデルの更新 ---
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
    }


    // 各Jointの位置をSphereRegionに反映
    boneLines_->ClearInstances();
    for (size_t i = 0; i < skeleton_.joints.size(); ++i) {
        // JointのSkeleton空間での行列を取得
        const Matrix4x4& jointMat = skeleton_.joints[i].skeletonSpaceMatrix;

        // Jointのワールド座標 = Joint(Skeleton空間) * モデルのWorld行列
        Matrix4x4 jointWorldMat = jointMat * worldMatrix_;

        // 行列から位置（translate）だけを抽出
        Vector3 jointPosition = {
            jointWorldMat.m[3][0],
            jointWorldMat.m[3][1],
            jointWorldMat.m[3][2]
        };

        // SphereRegionのi番目のインスタンスの位置を更新
        Transform tf;
        tf.scale = { 0.01f, 0.01f, 0.01f };
        tf.rotate = { 0.0f, 0.0f, 0.0f }; // 球体なので回転は無視でOK
        tf.translate = jointPosition;

        jointSpheres_->UpdateInstance(static_cast<uint32_t>(i), tf);

        // 親ジョイントがあれば、親から自分への線（ボーン）を描画
        if (skeleton_.joints[i].parent) {
            const int32_t parentIndex = *skeleton_.joints[i].parent;
            const Matrix4x4& parentMat = skeleton_.joints[parentIndex].skeletonSpaceMatrix;
            Matrix4x4 parentWorldMat = parentMat * worldMatrix_;
            Vector3 parentPosition = {
                parentWorldMat.m[3][0],
                parentWorldMat.m[3][1],
                parentWorldMat.m[3][2]
            };
            boneLines_->AddInstance(parentPosition, jointPosition, { 1.0f, 1.0f, 0.0f, 1.0f });
        }
    }

    // マテリアル情報をGPUへ転送
    UpdateMaterials();

}

// 描画
void AnimationModel::Draw() {

    if (!managedModel_ || !engine_) return;

    // --- 追加：骨格（球体の集合）を一括描画 ---
    if (jointSpheres_ && !skeleton_.joints.empty()) {
        engine_->ApplyRegionPSO();
        jointSpheres_->Draw();
    }

    // --- 追加：ボーン（線）を一括描画 ---
    if (boneLines_ && !skeleton_.joints.empty()) {
        engine_->ApplyLineInstancedPSO();
        boneLines_->Draw();
    }

    // スキニングの有無でPSOと描画関数を切り替え
    if (!managedModel_->cpuModel->skinClusterData.empty()) {
        engine_->ApplySkinningPSO();
        // モデルと、このオブジェクトが持つ変換行列リソースのGPUアドレスを渡して描画を依頼
        engine_->GetDrawManager()->DrawAnimationModel(managedModel_.get(), GetTransformationGpuAddress(), skinCluster_);
    } else {
        engine_->ApplyPSO();
        // メッシュごとに描画
        engine_->GetDrawManager()->DrawModel(managedModel_.get(), GetTransformationGpuAddress());
    }
}

// デバッグ
void AnimationModel::Debug([[maybe_unused]] const char* objName) {
#if defined USE_IMGUI
    std::string name = std::string("AnimationModel: ") + objName;
    ImGui::Begin(name.c_str());
    if (engine_) {
        engine_->GetDebugUI()->DebugTransform(transform_);

        // ImGuiでマテリアルを編集
        if (managedModel_ && managedModel_->cpuModel) {
            for (size_t i = 0; i < managedModel_->cpuModel->meshes.size(); ++i) {
                std::string materialLabel = "Mesh " + std::to_string(i) + " Material";
                if (ImGui::TreeNode(materialLabel.c_str())) {
                    ObjMaterial* mat = GetMaterial(i);
                    if (mat) {
                        // unique_id を渡してコントロールIDの衝突を避ける
                        std::string unique_id = "##" + std::to_string(i);
                        engine_->GetDebugUI()->DebugObjMaterial(mat, unique_id.c_str());
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

    // スキニングアニメーションの場合
    if (!managedModel_->cpuModel->skinClusterData.empty()) {
        // 1. 全Jointにアニメーションを適用
        AnimationManager::ApplyAnimation(skeleton_, animation_, animationTime_);

        // 2. 階層構造の行列更新
        AnimationManager::SkeletonUpdate(skeleton_);

        // 3. SkinClusterのMatrixPaletteを更新
        AnimationManager::SkinClusterUpdate(skinCluster_, skeleton_);
    } else { // ノードアニメーションの場合
        // rootNodeのAnimationを取得
        NodeAnimation& rootNodeAnimation = animation_.nodeAnimations[managedModel_->cpuModel->rootNode.name];
        // 指定時刻の値を取得
        Vector3 translate = AnimationManager::CalculateValue(rootNodeAnimation.translate, animationTime_);
        Quaternion rotate = AnimationManager::CalculateValue(rootNodeAnimation.rotate, animationTime_);
        Vector3 scale = AnimationManager::CalculateValue(rootNodeAnimation.scale, animationTime_);
        localMatrix_ = Math::MakeAffineMatrix(scale, rotate, translate);
    }
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