#include "Engine/Core/Utility/ErrorUtility.h"
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
#include "Renderer/Object/Batch/PrimitiveBatch.h"
#include "Renderer/Object/Line/LineClass.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include <cmath>
#include <cassert>



AnimationModel::AnimationModel() {}
AnimationModel::~AnimationModel() {
}

// 初期化
void AnimationModel::Initialize(const std::string& filename) {
    filename_ = filename;

    IRUFEMI_ASSERT(engine_ && "AnimationModel::Initialize: ModelManager is not set.");
    // 非同期で読み込みを開始し、メインスレッドをブロックしない
    modelHandle_ = engine_->GetObjModelManager()->LoadModel(filename);

    // StatusがLoadedであれば直ちに初期化を試みる
    if (auto m = engine_->GetObjModelManager()->Resolve(modelHandle_)) {
        auto status = m->status.load();
        if (status == ManagedModel::LoadingStatus::Loaded && m->cpuModel) {
            InitializeResources();
        }
    }
}

void AnimationModel::InitializeResources() {
    auto m = engine_ ? engine_->GetObjModelManager()->Resolve(modelHandle_) : nullptr;
    if (!m || !m->cpuModel) {
        return;
    }

    // 4. 変換行列リソースの生成とマップ (全メッシュ共有用)
    IRUFEMI_ASSERT(engine_->GetDrawManager() && "DrawManager is not set.");
    if (transformCbIndex_ == static_cast<uint32_t>(-1)) {
        transformCbIndex_ = engine_->GetTransformBufferManager()->Allocate();
    }

    // 各メッシュ用リソースの生成
    meshResources_.clear();
    for (size_t i = 0; i < m->gpuMeshes.size(); ++i) {
        auto res = std::make_unique<Object3DResource>();
        
        res->SetExternalTransformCbIndex(&transformCbIndex_);
        
        const auto& gpuMesh = m->gpuMeshes[i];
        res->vertexBufferView_ = gpuMesh->vertexBufferView;
        res->indexBufferView_ = gpuMesh->indexBufferView;
        res->indexCount_ = gpuMesh->indexCount;
        
        res->CreateResource();

        // 初期テクスチャハンドルをコピー
        const auto& gpuMaterial = (i < m->gpuMaterials.size()) ? m->gpuMaterials[i] : nullptr;
        if (gpuMaterial) {
            res->textureHandle_ = gpuMaterial->textureHandle;
        }

        meshResources_.push_back(std::move(res));
    }

    IRUFEMI_ASSERT(engine_ && "AnimationModel::Initialize: AnimationManager is not set.");
    animation_ = engine_->GetAnimationManager()->LoadAnimationFile(filename_);

    // Skeletonの生成
    if (m && m->cpuModel) {
        skeleton_ = AnimationManager::CreateSkeleton(m->cpuModel->rootNode);

        if (!m->cpuModel->skinClusterData.empty()) {
            skinCluster_ = engine_->GetAnimationManager()->CreateSkinCluster(skeleton_, *m->cpuModel);
        }

        jointSpheres_ = std::make_unique<PrimitiveBatch>();
        jointSpheres_->Initialize(PrimitiveType::Sphere, "resources/whiteTexture.png");

        for (size_t i = 0; i < skeleton_.joints.size(); ++i) {
            Transform tf{};
            tf.scale = { 0.02f, 0.02f, 0.02f };
            jointSpheres_->AddInstance(tf);
        }

        boneLines_ = std::make_unique<Line3DBatch>();
        boneLines_->Initialize();
    }

    animationTime_ = 0.0f;
    Update();
}

// 更新
void AnimationModel::Update() {

    auto m = engine_ ? engine_->GetObjModelManager()->Resolve(modelHandle_) : nullptr;
    if (!m || !engine_) return;
    Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    // 非同期ロードが終わっていれば構築する (遅延初期化)
    if (m->status.load() == ManagedModel::LoadingStatus::Loaded && meshResources_.empty()) {
        InitializeResources();
    }

    // まだ準備できていない場合はスキップ
    if (meshResources_.empty()) return;

    worldMatrix_ = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    UpdateAnimation();

    // スキニングモデルかノードアニメーションモデルかで処理を分岐
    if (!m->cpuModel->skinClusterData.empty()) {
        // --- スキニングモデルの更新 ---
        transformationMatrix_.world = worldMatrix_;
        Matrix4x4 worldForNormal = transformationMatrix_.world;
        worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f;
        worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
        transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
        
        // SyncBeforeDraw() で同期するため、ここでは計算のみ行う
    } else {
        // --- ノードアニメーションモデルの更新 ---
        // オブジェクト全体のワールド行列を計算
        transformationMatrix_.world = localMatrix_ * worldMatrix_;

        // 法線変換用の逆転置行列
        Matrix4x4 worldForNormal = transformationMatrix_.world;
        worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f;
        worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
        transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

        // 計算した行列をマップ済みのリソースにコピー
        // SyncBeforeDraw() で同期するため、ここでは計算のみ行う
    }


    // 各Jointの位置をSphereBatchに反映
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

        // SphereBatchのi番目のインスタンスの位置を更新
        Transform tf;
        tf.scale = { 0.02f, 0.02f, 0.02f };
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
    lastViewMatrix_ = activeCam->GetViewMatrix();
    lastProjectionMatrix_ = activeCam->GetPerspectiveFovMatrix();
    // スキニングモデルの場合はCompute Shaderの実行を予約する
    if (!m->cpuModel->skinClusterData.empty() && engine_ && engine_->GetDrawManager()) {
        engine_->GetDrawManager()->RegisterComputeTask(this);
    }
}

void AnimationModel::SyncBeforeDraw() {
    uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
    
    if (lastUpdateFrame_ == frameIndex) {
        return;
    }
    
    // 変換行列の更新 (全メッシュで共有)
    if (transformCbIndex_ != static_cast<uint32_t>(-1)) {
        engine_->GetTransformBufferManager()->Update(transformCbIndex_, transformationMatrix_, frameIndex);
    }
    
    // 各メッシュのマテリアル等の更新
    for (auto& res : meshResources_) {
        res->SyncBeforeDraw();
    }
    
    // --- SkinCluster のマルチバッファ同期（ポーズ中の振動対策） ---
    auto m = engine_ ? engine_->GetObjModelManager()->Resolve(modelHandle_) : nullptr;
    if (m && m->cpuModel && !m->cpuModel->skinClusterData.empty()) {
        AnimationManager::SkinClusterUpdate(skinCluster_, skeleton_, frameIndex);
    }
    
    lastUpdateFrame_ = frameIndex;
}

void AnimationModel::DispatchCompute() {
    auto m = engine_ ? engine_->GetObjModelManager()->Resolve(modelHandle_) : nullptr;
    if (!m || !m->cpuModel || m->cpuModel->skinClusterData.empty() || !engine_) return;
    Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    if (isCullingEnabled_) {
        float maxScale = (std::max)({ transform_.scale.x, transform_.scale.y, transform_.scale.z });
        Sphere boundingSphere;
        boundingSphere.center = transform_.translate;
        boundingSphere.radius = m->cpuModel->boundingSphere.radius * maxScale * 1.5f;
        if (!Collision::IsCollision(activeCam->GetFrustum(), boundingSphere)) {
            return; // 視錐台カリングされている場合はComputeもスキップ
        }
    }

    engine_->GetDrawManager()->DispatchSkinning(skinCluster_, m, skinCluster_.mappedSkinningInformation->numVertices);
    uint32_t f = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
    
    // スキニングが正常に実行されたフレームを記録（ポーズ中の遅延更新等による不整合を防ぐため）
    lastSkinnedFrameIndex_ = f;
}

// 描画
void AnimationModel::Draw() {
    auto m = engine_ ? engine_->GetObjModelManager()->Resolve(modelHandle_) : nullptr;
    if (!m || !engine_ || meshResources_.empty()) return;
    Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    // 視錐台カリング
    if (isCullingEnabled_) {
        float maxScale = (std::max)({ transform_.scale.x, transform_.scale.y, transform_.scale.z });
        
        Sphere boundingSphere;
        boundingSphere.center = transform_.translate;
        // アニメーションによる広がりを考慮し、モデル境界球の1.5倍のマージンを設定
        boundingSphere.radius = m->cpuModel->boundingSphere.radius * maxScale * 1.5f;

        if (!Collision::IsCollision(activeCam->GetFrustum(), boundingSphere)) {
            return; // 描画スキップ
        }
    }

    // カメラの行列が変更されたか、オブジェクト自体が変更されたかチェック
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &activeCam->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &activeCam->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirtyBuffer_[BaseResource::GetDirectXCommon()->GetFrameIndex()] || cameraChanged) {
        Update();
    }
    
    // --- 【追加】描画直前のバッファ同期 ---
    SyncBeforeDraw();

    // --- 追加：骨格（球体の集合）を一括描画 ---
    if (jointSpheres_ && !skeleton_.joints.empty()) {
        jointSpheres_->Draw();
    }

    // --- 追加：ボーン（線）を一括描画 ---
    if (boneLines_ && !skeleton_.joints.empty()) {
        boneLines_->Draw();
    }

    // 2. グラフィックスPSOの適用はDrawManagerが行うため削除

    // 3. 全メッシュをループして描画
    uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
    for (size_t i = 0; i < meshResources_.size(); ++i) {
        auto& res = meshResources_[i];

        // スキニング中なら VBV を差し替えて描画
        if (!m->cpuModel->skinClusterData.empty()) {
            engine_->GetDrawManager()->SubmitStandard3D(res.get(), &skinCluster_.skinnedVertexBufferView[lastSkinnedFrameIndex_], castShadows_, skinCluster_.skinnedVertexResource[lastSkinnedFrameIndex_].Get());
        } else {
            engine_->GetDrawManager()->SubmitStandard3D(res.get(), nullptr, castShadows_);
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
        if (animation_) {
            ui_->DebugAnimationControl(*animation_, animationTime_);
        }
        ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);

        if (ImGui::Button("Reset Animation Time")) {
            animationTime_ = 0.0f;
        }

        ImGui::ColorEdit4("Color", &color_.x); // インスタンスカラーを編集
        ui_->DebugMaterialOverrides(&environmentCoefficient_, &lightingModeOverride_, &useClampSamplerOverride_, &enableLightingOverride_, "##AmOverrides");

        // ImGuiでマテリアルを編集
        auto m = engine_->GetObjModelManager()->Resolve(modelHandle_);
        if (m && m->cpuModel) {
            for (size_t i = 0; i < m->cpuModel->meshes.size(); ++i) {
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




void AnimationModel::UpdateAnimation() {

    // アニメーションの処理
    if (!animation_) return;

    // 時刻を進める（タイムスケール対応）
    animationTime_ += engine_->GetGameDeltaTime();
    if (animation_->duration > 0.0f) {
        animationTime_ = std::fmod(animationTime_, animation_->duration); // 最後まで行ったら最初からリピート再生。
    }

    auto m = engine_ ? engine_->GetObjModelManager()->Resolve(modelHandle_) : nullptr;
    if (!m || !m->cpuModel) return;

    // スキニングアニメーションの場合
    if (!m->cpuModel->skinClusterData.empty()) {
        // 1. 全Jointにアニメーションを適用
        AnimationManager::ApplyAnimation(skeleton_, *animation_, animationTime_);

        // 2. 階層構造の行列更新
        AnimationManager::SkeletonUpdate(skeleton_);

        // 3. SkinClusterのMatrixPaletteを更新
        uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
        AnimationManager::SkinClusterUpdate(skinCluster_, skeleton_, frameIndex);
    } else { // ノードアニメーションの場合
        // rootNodeのAnimationを取得
        NodeAnimation& rootNodeAnimation = animation_->nodeAnimations[m->cpuModel->rootNode.name];
        // 指定時刻の値を取得
        Vector3 translate = AnimationManager::CalculateValue(rootNodeAnimation.translate, animationTime_);
        Quaternion rotate = AnimationManager::CalculateValue(rootNodeAnimation.rotate, animationTime_);
        Vector3 scale = AnimationManager::CalculateValue(rootNodeAnimation.scale, animationTime_);
        localMatrix_ = Math::MakeAffineMatrix(scale, rotate, translate);
    }
}

