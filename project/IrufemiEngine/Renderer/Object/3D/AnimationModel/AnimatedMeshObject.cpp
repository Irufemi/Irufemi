#include "AnimatedMeshObject.h"
#include "Engine/Core/Utility/ErrorUtility.h"
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

AnimatedMeshObject::AnimatedMeshObject() {}
AnimatedMeshObject::~AnimatedMeshObject() {}

void AnimatedMeshObject::Initialize(const std::string& filename) {
    filename_ = filename;

    IRUFEMI_ASSERT(engine_ && "AnimatedMeshObject::Initialize: ModelManager is not set.");
    modelHandle_ = engine_->GetObjModelManager()->LoadModel(filename);

    meshResources_.clear(); // モデル切り替え時に古いリソースを確実に破棄する

    if (auto m = engine_->GetObjModelManager()->Resolve(modelHandle_)) {
        auto status = m->status.load();
        if (status == ManagedModel::LoadingStatus::Loaded && m->cpuModel) {
            InitializeResources();
        }
    }
}

void AnimatedMeshObject::InitializeResources() {
    auto m = engine_ ? engine_->GetObjModelManager()->Resolve(modelHandle_) : nullptr;
    if (!m || !m->cpuModel) return;

    if (transformCbIndex_ == static_cast<uint32_t>(-1)) {
        transformCbIndex_ = engine_->GetTransformBufferManager()->Allocate();
    }

    meshResources_.clear();
    for (size_t i = 0; i < m->gpuMeshes.size(); ++i) {
        auto res = std::make_unique<Object3DResource>();
        res->SetExternalTransformCbIndex(&transformCbIndex_);
        
        const auto& gpuMesh = m->gpuMeshes[i];
        res->vertexBufferView_ = gpuMesh->vertexBufferView;
        res->indexBufferView_ = gpuMesh->indexBufferView;
        res->indexCount_ = gpuMesh->indexCount;
        
        res->CreateResource();

        const auto& gpuMaterial = (i < m->gpuMaterials.size()) ? m->gpuMaterials[i] : nullptr;
        if (gpuMaterial) {
            res->textureHandle_ = gpuMaterial->textureHandle;
        }
        meshResources_.push_back(std::move(res));
    }

    if (m && m->cpuModel) {
        skeletonData_ = AnimationManager::CreateSkeletonData(m->cpuModel->rootNode);
        internalPose_ = AnimationManager::CreateSkeletonPose(&skeletonData_);
        currentPose_ = &internalPose_;

        if (!m->cpuModel->skinClusterData.empty()) {
            skinCluster_ = engine_->GetAnimationManager()->CreateSkinCluster(skeletonData_, *m->cpuModel);
        }

        jointSpheres_ = std::make_unique<PrimitiveBatch>();
        jointSpheres_->Initialize(PrimitiveType::Sphere, "resources/whiteTexture.png");
        jointSpheres_->SetDepthWrite(PSOManager::DepthWrite::Off);

        for (size_t i = 0; i < skeletonData_.joints.size(); ++i) {
            Transform tf{};
            tf.scale = { 0.02f, 0.02f, 0.02f };
            jointSpheres_->AddInstance(tf);
        }

        boneMeshes_ = std::make_unique<PrimitiveBatch>();
        boneMeshes_->Initialize(PrimitiveType::Octahedron, "resources/whiteTexture.png");
        boneMeshes_->SetDepthWrite(PSOManager::DepthWrite::Off);
        
        debugAxesLines_ = std::make_unique<Line3DBatch>();
        debugAxesLines_->Initialize();
        debugAxesLines_->SetDepthWrite(PSOManager::DepthWrite::Off);
    }

    Update();
}

void AnimatedMeshObject::Update(const SkeletonPose* externalPose) {
    auto m = engine_ ? engine_->GetObjModelManager()->Resolve(modelHandle_) : nullptr;
    if (!m || !engine_) return;

    Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    if (m->status.load() != ManagedModel::LoadingStatus::Loaded) return;

    if (meshResources_.empty()) {
        InitializeResources();
    }
    if (meshResources_.empty()) return;

    // ポーズの差し替え
    if (externalPose) {
        currentPose_ = externalPose;
    } else {
        currentPose_ = &internalPose_;
    }

    worldMatrix_ = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    if (!m->cpuModel->skinClusterData.empty()) {
        transformationMatrix_.world = worldMatrix_;
        Matrix4x4 worldForNormal = transformationMatrix_.world;
        worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f;
        worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
        transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
    } else {
        transformationMatrix_.world = localMatrix_ * worldMatrix_;
        Matrix4x4 worldForNormal = transformationMatrix_.world;
        worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f;
        worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
        transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
    }

    // デバッグ表示用
    boneMeshes_->ClearInstances();
    if (isDebugBoneVisible_ && currentPose_ && currentPose_->data) {
        for (size_t i = 0; i < currentPose_->jointPoses.size(); ++i) {
            const Matrix4x4& jointMat = currentPose_->jointPoses[i].skeletonSpaceMatrix;
            Matrix4x4 jointWorldMat = jointMat * worldMatrix_;
            Vector3 jointPosition = { jointWorldMat.m[3][0], jointWorldMat.m[3][1], jointWorldMat.m[3][2] };

            Transform tf;
            tf.scale = { 0.02f, 0.02f, 0.02f };
            tf.rotate = { 0.0f, 0.0f, 0.0f };
            tf.translate = jointPosition;
            jointSpheres_->UpdateInstance(static_cast<uint32_t>(i), tf);

            if (currentPose_->data->joints[i].parent) {
                const int32_t parentIndex = *currentPose_->data->joints[i].parent;
                const Matrix4x4& parentMat = currentPose_->jointPoses[parentIndex].skeletonSpaceMatrix;
                Matrix4x4 parentWorldMat = parentMat * worldMatrix_;
                Vector3 parentPosition = { parentWorldMat.m[3][0], parentWorldMat.m[3][1], parentWorldMat.m[3][2] };
                
                Vector3 dir = { jointPosition.x - parentPosition.x, jointPosition.y - parentPosition.y, jointPosition.z - parentPosition.z };
                float length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
                
                if (length > 0.0f) {
                    Vector3 yAxis = { dir.x / length, dir.y / length, dir.z / length };
                    Vector3 xAxis;
                    if (std::abs(yAxis.y) > 0.999f) {
                        xAxis = {1.0f, 0.0f, 0.0f};
                    } else {
                        Vector3 up = {0.0f, 1.0f, 0.0f};
                        xAxis = {up.y * yAxis.z - up.z * yAxis.y, up.z * yAxis.x - up.x * yAxis.z, up.x * yAxis.y - up.y * yAxis.x};
                        float xl = std::sqrt(xAxis.x * xAxis.x + xAxis.y * xAxis.y + xAxis.z * xAxis.z);
                        xAxis.x /= xl; xAxis.y /= xl; xAxis.z /= xl;
                    }
                    Vector3 zAxis = {xAxis.y * yAxis.z - xAxis.z * yAxis.y, xAxis.z * yAxis.x - xAxis.x * yAxis.z, xAxis.x * yAxis.y - xAxis.y * yAxis.x};

                    float thickness = length * 0.1f;
                    xAxis.x *= thickness; xAxis.y *= thickness; xAxis.z *= thickness;
                    yAxis.x *= length;    yAxis.y *= length;    yAxis.z *= length;
                    zAxis.x *= thickness; zAxis.y *= thickness; zAxis.z *= thickness;

                    Matrix4x4 boneWorld = {
                        xAxis.x, xAxis.y, xAxis.z, 0.0f,
                        yAxis.x, yAxis.y, yAxis.z, 0.0f,
                        zAxis.x, zAxis.y, zAxis.z, 0.0f,
                        parentPosition.x, parentPosition.y, parentPosition.z, 1.0f
                    };

                    int depth = 0;
                    int32_t curr = parentIndex;
                    while (currentPose_->data->joints[curr].parent) {
                        depth++;
                        curr = *currentPose_->data->joints[curr].parent;
                    }

                    float hue = std::fmod(depth * 30.0f, 360.0f);
                    float c = 1.0f;
                    float x = c * (1.0f - std::abs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
                    Vector4 color = {0, 0, 0, 0.6f};
                    if (hue < 60)      { color.x = c; color.y = x; color.z = 0; }
                    else if (hue < 120){ color.x = x; color.y = c; color.z = 0; }
                    else if (hue < 180){ color.x = 0; color.y = c; color.z = x; }
                    else if (hue < 240){ color.x = 0; color.y = x; color.z = c; }
                    else if (hue < 300){ color.x = x; color.y = 0; color.z = c; }
                    else               { color.x = c; color.y = 0; color.z = x; }
                    boneMeshes_->AddInstanceWorld(boneWorld, color);
                }
            }

            // --- 選択されたボーンのローカル座標系（XYZ軸）を描画 ---
            if (debugAxesLines_ && static_cast<int32_t>(i) == selectedJointIndex_) {
                debugAxesLines_->ClearInstances();
                
                const Matrix4x4& jointMat = currentPose_->jointPoses[i].skeletonSpaceMatrix;
                Matrix4x4 jointWorldMat = jointMat * worldMatrix_;
                Vector3 origin = { jointWorldMat.m[3][0], jointWorldMat.m[3][1], jointWorldMat.m[3][2] };
                
                // ワールド空間上でのローカルXYZ軸方向ベクトルを抽出
                Vector3 axisX = { jointWorldMat.m[0][0], jointWorldMat.m[0][1], jointWorldMat.m[0][2] };
                Vector3 axisY = { jointWorldMat.m[1][0], jointWorldMat.m[1][1], jointWorldMat.m[1][2] };
                Vector3 axisZ = { jointWorldMat.m[2][0], jointWorldMat.m[2][1], jointWorldMat.m[2][2] };
                
                // 正規化
                auto Normalize = [](Vector3& v) {
                    float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
                    if (l > 0.0f) { v.x /= l; v.y /= l; v.z /= l; }
                };
                Normalize(axisX);
                Normalize(axisY);
                Normalize(axisZ);

                float lineLen = 0.5f; // 軸の長さ
                debugAxesLines_->AddInstance(origin, { origin.x + axisX.x * lineLen, origin.y + axisX.y * lineLen, origin.z + axisX.z * lineLen }, { 1.0f, 0.0f, 0.0f, 1.0f });
                debugAxesLines_->AddInstance(origin, { origin.x + axisY.x * lineLen, origin.y + axisY.y * lineLen, origin.z + axisY.z * lineLen }, { 0.0f, 1.0f, 0.0f, 1.0f });
                debugAxesLines_->AddInstance(origin, { origin.x + axisZ.x * lineLen, origin.y + axisZ.y * lineLen, origin.z + axisZ.z * lineLen }, { 0.0f, 0.5f, 1.0f, 1.0f });
            }
        }
        if (selectedJointIndex_ < 0 && debugAxesLines_) {
            debugAxesLines_->ClearInstances();
        }
    }

    UpdateMaterials();
    isDirty_ = false;
    lastViewMatrix_ = activeCam->GetViewMatrix();
    lastProjectionMatrix_ = activeCam->GetPerspectiveFovMatrix();

    if (!m->cpuModel->skinClusterData.empty() && engine_ && engine_->GetDrawManager()) {
        engine_->GetDrawManager()->RegisterComputeTask(this);
    }
}

void AnimatedMeshObject::SyncBeforeDraw() {
    uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
    
    if (transformCbIndex_ != static_cast<uint32_t>(-1)) {
        engine_->GetTransformBufferManager()->Update(transformCbIndex_, transformationMatrix_, frameIndex);
    }
    
    for (auto& res : meshResources_) {
        res->SyncBeforeDraw();
    }
    
    if (jointSpheres_) jointSpheres_->SyncBeforeDraw();
    if (boneMeshes_) boneMeshes_->SyncBeforeDraw();
    if (debugAxesLines_) debugAxesLines_->SyncBeforeDraw();

    auto m = engine_ ? engine_->GetObjModelManager()->Resolve(modelHandle_) : nullptr;
    if (m && m->cpuModel && !m->cpuModel->skinClusterData.empty() && currentPose_) {
        AnimationManager::SkinClusterUpdate(skinCluster_, *currentPose_, frameIndex);
    }
}

void AnimatedMeshObject::DispatchCompute() {
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
            return; 
        }
    }

    engine_->GetDrawManager()->DispatchSkinning(skinCluster_, m, skinCluster_.mappedSkinningInformation->numVertices);
    lastSkinnedFrameIndex_ = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
}

void AnimatedMeshObject::Draw() {
    auto m = engine_ ? engine_->GetObjModelManager()->Resolve(modelHandle_) : nullptr;
    if (!m || !m->cpuModel || !engine_ || meshResources_.empty()) return;
    Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    if (isCullingEnabled_) {
        float maxScale = (std::max)({ transform_.scale.x, transform_.scale.y, transform_.scale.z });
        Sphere boundingSphere;
        boundingSphere.center = transform_.translate;
        boundingSphere.radius = m->cpuModel->boundingSphere.radius * maxScale * 1.5f;
        if (!Collision::IsCollision(activeCam->GetFrustum(), boundingSphere)) return;
    }

    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &activeCam->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &activeCam->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirtyBuffer_[BaseResource::GetDirectXCommon()->GetFrameIndex()] || cameraChanged) {
        Update(currentPose_);
    }
    
    SyncBeforeDraw();

    if (isDebugBoneVisible_ && jointSpheres_ && currentPose_ && !currentPose_->jointPoses.empty()) {
        jointSpheres_->Draw();
    }
    if (isDebugBoneVisible_ && boneMeshes_ && currentPose_ && !currentPose_->jointPoses.empty()) {
        boneMeshes_->Draw();
    }
    if (isDebugBoneVisible_ && debugAxesLines_ && selectedJointIndex_ >= 0) {
        debugAxesLines_->Draw();
    }

    for (size_t i = 0; i < meshResources_.size(); ++i) {
        auto& res = meshResources_[i];
        if (!m->cpuModel->skinClusterData.empty()) {
            engine_->GetDrawManager()->SubmitStandard3D(res.get(), &skinCluster_.skinnedVertexBufferView[lastSkinnedFrameIndex_], castShadows_, skinCluster_.skinnedVertexResource[lastSkinnedFrameIndex_].Get());
        } else {
            engine_->GetDrawManager()->SubmitStandard3D(res.get(), nullptr, castShadows_);
        }
    }
}

void AnimatedMeshObject::DrawOutlineMask() {
    auto m = engine_ ? engine_->GetObjModelManager()->Resolve(modelHandle_) : nullptr;
    if (!m || !engine_ || !engine_->GetDrawManager() || meshResources_.empty()) return;
    for (auto& res : meshResources_) {
        engine_->GetDrawManager()->SubmitOutlineMask(res.get(), nullptr);
    }
}

void AnimatedMeshObject::Debug([[maybe_unused]] const char* objName) {
#if defined USE_IMGUI
    // ポインタアドレスをIDとして含めることで、同じモデルを使用する複数オブジェクトでImGuiのID衝突（およびそれに伴う頂点バッファ崩壊クラッシュ）を防ぐ
    std::string name = std::string("AnimMesh: ") + objName + "###AnimMesh_" + std::to_string(reinterpret_cast<uintptr_t>(this));
    ImGui::Begin(name.c_str());
    if (engine_) {
        auto* ui_ = engine_->GetDebugUI();
        ui_->DebugTransform(transform_);
        ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);
        ImGui::ColorEdit4("Color", &color_.x); 
        ui_->DebugMaterialOverrides(&environmentCoefficient_, &lightingModeOverride_, &useClampSamplerOverride_, &enableLightingOverride_, "##AmOverrides");
        
        ImGui::Separator();
        ImGui::Checkbox("Show Debug Bones", &isDebugBoneVisible_);
        if (isDebugBoneVisible_ && !skeletonData_.joints.empty()) {
            ImGui::Text("Skeleton Tree:");
            ImGui::BeginChild("SkeletonTreeRegion", ImVec2(0, 200), true);
            DrawSkeletonTreeRecursive(skeletonData_.root);
            ImGui::EndChild();
        }
    }
    ImGui::End();
#endif
}

void AnimatedMeshObject::DrawSkeletonTreeRecursive(int32_t jointIndex) {
#if defined USE_IMGUI
    if (jointIndex < 0 || static_cast<size_t>(jointIndex) >= skeletonData_.joints.size()) return;
    const auto& joint = skeletonData_.joints[jointIndex];

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (joint.children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (selectedJointIndex_ == jointIndex) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)jointIndex, flags, "%s", joint.name.c_str());
    if (ImGui::IsItemClicked()) {
        selectedJointIndex_ = jointIndex;
    }

    if (nodeOpen) {
        for (int32_t childIndex : joint.children) {
            DrawSkeletonTreeRecursive(childIndex);
        }
        ImGui::TreePop();
    }
#endif
}

const SkeletonData* AnimatedMeshObject::GetSkeletonData() const {
    auto m = engine_ ? engine_->GetObjModelManager()->Resolve(modelHandle_) : nullptr;
    if (!m || !m->cpuModel || m->cpuModel->skinClusterData.empty()) return nullptr;
    return &skeletonData_;
}

SkeletonPose* AnimatedMeshObject::GetInternalSkeletonPose() {
    return &internalPose_;
}
