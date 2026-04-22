#include "AnimationModel.h"

#include "Engine/IrufemiEngine.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Core/Math/Geometry/Frustum.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Shape/Sphere.h"
#include "Resource/Model/ModelManager.h"
#include "Resource/Model/AnimationManager.h"
#include "Resource/Model/Data/ObjModel.h"
#include "Renderer/Region/Primitive/SphereRegion.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "Application/camera/Camera.h"
#include <cmath>
#include <cassert>

// 静的メンバ定義
IrufemiEngine* AnimationModel::engine_ = nullptr;

AnimationModel::AnimationModel() {}
AnimationModel::~AnimationModel() {
}

// 初期化
void AnimationModel::Initialize(Camera* camera, const std::string& filename) {
    camera_ = camera;
    filename_ = filename;

    assert(engine_ && "AnimationModel::Initialize: ModelManager is not set.");
    // 非同期で読み込みを開始し、メインスレッドをブロックしない
    managedModel_ = engine_->GetObjModelManager()->GetModelAsync(filename);

    // StatusがLoadedであれば直ちに初期化を試みる
    auto status = managedModel_->status.load();
    if (status == ManagedModel::LoadingStatus::Loaded && managedModel_->cpuModel) {
        InitializeResources();
    }
}

void AnimationModel::InitializeResources() {
    if (!managedModel_ || !managedModel_->cpuModel) {
        return;
    }

    // 4. 変換行列リソースの生成とマップ (全メッシュ共有用)
    assert(engine_->GetDrawManager() && "DrawManager is not set.");
    transformationBuffer_.Initialize(engine_->GetDrawManager()->GetDxCommon());

    // 各メッシュ用リソースの生成
    meshResources_.clear();
    for (size_t i = 0; i < managedModel_->gpuMeshes.size(); ++i) {
        auto res = std::make_unique<Object3DResource>();
        
        res->SetExternalTransformationBuffer(&transformationBuffer_);
        
        const auto& gpuMesh = managedModel_->gpuMeshes[i];
        res->vertexBufferView_ = gpuMesh->vertexBufferView;
        res->indexBufferView_ = gpuMesh->indexBufferView;
        res->indexCount_ = gpuMesh->indexCount;
        
        res->CreateResource();

        // 初期テクスチャハンドルをコピー
        const auto& gpuMaterial = (i < managedModel_->gpuMaterials.size()) ? managedModel_->gpuMaterials[i] : nullptr;
        if (gpuMaterial) {
            res->textureHandle_ = gpuMaterial->textureHandle;
        }

        meshResources_.push_back(std::move(res));
    }

    assert(engine_ && "AnimationModel::Initialize: AnimationManager is not set.");
    animation_ = engine_->GetAnimationManager()->LoadAnimationFile(filename_);

    // Skeletonの生成
    if (managedModel_ && managedModel_->cpuModel) {
        skeleton_ = AnimationManager::CreateSkeleton(managedModel_->cpuModel->rootNode);

        if (!managedModel_->cpuModel->skinClusterData.empty()) {
            skinCluster_ = engine_->GetAnimationManager()->CreateSkinCluster(skeleton_, *managedModel_->cpuModel);
        }

        jointSpheres_ = std::make_unique<SphereRegion>();
        jointSpheres_->Initialize(camera_, "resources/whiteTexture.png", 16);

        for (size_t i = 0; i < skeleton_.joints.size(); ++i) {
            Transform tf{};
            tf.scale = { 0.01f, 0.01f, 0.01f };
            jointSpheres_->AddInstance(tf);
        }

        boneLines_ = std::make_unique<Line3DRegion>();
        boneLines_->Initialize(camera_);
    }

    animationTime_ = 0.0f;
    Update();
}

// 更新
void AnimationModel::Update() {

    if (!managedModel_ || !camera_) return;

    // 非同期ロードが終わっていれば構築する (遅延初期化)
    if (managedModel_->status.load() == ManagedModel::LoadingStatus::Loaded && meshResources_.empty()) {
        InitializeResources();
    }

    // まだ準備できていない場合はスキップ
    if (meshResources_.empty()) return;

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
        transformationBuffer_.Update(transformationMatrix_, engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex());
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
        transformationBuffer_.Update(transformationMatrix_, engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex());
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

    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
    
    MakeDirty();
}

void AnimationModel::SyncIfDirty() {
    if (dirtyFramesLeft_ > 0) {
        uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
        transformationBuffer_.Update(transformationMatrix_, frameIndex);
        for (auto& res : meshResources_) {
            res->SyncMaterialData();
        }
        
        // --- 追加: SkinCluster のマルチバッファ同期（ポーズ中の振動対策） ---
        if (managedModel_ && managedModel_->cpuModel && !managedModel_->cpuModel->skinClusterData.empty()) {
            AnimationManager::SkinClusterUpdate(skinCluster_, skeleton_, frameIndex);
        }
        
        if (lastSyncedFrameIndex_ != frameIndex) {
            dirtyFramesLeft_--;
            lastSyncedFrameIndex_ = frameIndex;
        }
    }
}

// 描画
void AnimationModel::Draw() {
    if (!managedModel_ || !camera_ || meshResources_.empty()) return;

    // 視錐台カリング
    if (isCullingEnabled_) {
        float maxScale = (std::max)({ transform_.scale.x, transform_.scale.y, transform_.scale.z });
        
        Sphere boundingSphere;
        boundingSphere.center = transform_.translate;
        // アニメーションによる広がりを考慮し、モデル境界球の1.5倍のマージンを設定
        boundingSphere.radius = managedModel_->cpuModel->boundingSphere.radius * maxScale * 1.5f;

        if (!Collision::IsCollision(camera_->GetFrustum(), boundingSphere)) {
            return; // 描画スキップ
        }
    }

    // カメラの行列が変更されたか、オブジェクト自体が変更されたかチェック
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &camera_->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &camera_->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirty_ || cameraChanged) {
        Update();
    }
    
    SyncIfDirty();

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

    // 1. スキニングの実行
    if (!managedModel_->cpuModel->skinClusterData.empty()) {
        engine_->GetDrawManager()->DispatchSkinning(skinCluster_, managedModel_.get(), skinCluster_.mappedSkinningInformation->numVertices);
        uint32_t f = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
        engine_->GetDrawManager()->ExecuteUAVBarrier(skinCluster_.skinnedVertexResource[f].Get());
    }

    // 2. グラフィックスPSOの適用
    engine_->ApplyPSO();

    // 3. 全メッシュをループして描画
    uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
    for (size_t i = 0; i < meshResources_.size(); ++i) {
        auto& res = meshResources_[i];

        // スキニング中なら VBV を差し替えて描画
        if (!managedModel_->cpuModel->skinClusterData.empty()) {
            engine_->GetDrawManager()->DrawStandard3D(res.get(), &skinCluster_.skinnedVertexBufferView[frameIndex]);
        } else {
            engine_->GetDrawManager()->DrawStandard3D(res.get());
        }
    }
}

// デバッグ
void AnimationModel::Debug([[maybe_unused]] const char* objName) {
#if defined USE_IMGUI
    std::string name = std::string("AnimationModel: ") + objName;
    ImGui::Begin(name.c_str());
    if (engine_) {
        auto* ui_ = engine_->GetDebugUI();
        ui_->DebugTransform(transform_);
        ui_->DebugAnimationControl(animation_, animationTime_);
        ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);

        if (ImGui::Button("Reset Animation Time")) {
            animationTime_ = 0.0f;
        }

        ImGui::ColorEdit4("Color", &color_.x); // インスタンスカラーを編集
        ui_->DebugMaterialOverrides(&environmentCoefficient_, &lightingModeOverride_, &useClampSamplerOverride_, &enableLightingOverride_, "##AmOverrides");

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
    if (!managedModel_ || !managedModel_->cpuModel || meshResources_.empty()) {
        return;
    }

    // 全メッシュのマテリアルを更新
    for (size_t i = 0; i < managedModel_->cpuModel->meshes.size(); ++i) {
        if (i >= meshResources_.size()) break;

        auto& res = meshResources_[i];
        if (!res->GetMaterialData()) continue;

        const ObjMaterial& cpuMat = managedModel_->cpuModel->meshes[i].material;
        Material* mappedData = res->GetMaterialData();

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
        
        // アルファテスト用閾値
        mappedData->alphaReference = cpuMat.alphaReference;

        res->SyncMaterialData();
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
        uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
        AnimationManager::SkinClusterUpdate(skinCluster_, skeleton_, frameIndex);
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