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

        for (size_t i = 0; i < skeletonData_.joints.size(); ++i) {
            Transform tf{};
            tf.scale = { 0.02f, 0.02f, 0.02f };
            jointSpheres_->AddInstance(tf);
        }

        boneLines_ = std::make_unique<Line3DBatch>();
        boneLines_->Initialize();
    }

    Update();
}

void AnimatedMeshObject::Update(const SkeletonPose* externalPose) {
    auto m = engine_ ? engine_->GetObjModelManager()->Resolve(modelHandle_) : nullptr;
    if (!m || !engine_) return;
    Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    if (m->status.load() == ManagedModel::LoadingStatus::Loaded && meshResources_.empty()) {
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
    boneLines_->ClearInstances();
    if (currentPose_ && currentPose_->data) {
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
                boneLines_->AddInstance(parentPosition, jointPosition, { 1.0f, 1.0f, 0.0f, 1.0f });
            }
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
    if (!m || !engine_ || meshResources_.empty()) return;
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

    if (jointSpheres_ && currentPose_ && !currentPose_->jointPoses.empty()) {
        jointSpheres_->Draw();
    }
    if (boneLines_ && currentPose_ && !currentPose_->jointPoses.empty()) {
        boneLines_->Draw();
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
    std::string name = std::string("AnimatedMeshObject: ") + objName;
    ImGui::Begin(name.c_str());
    if (engine_) {
        auto* ui_ = engine_->GetDebugUI();
        ui_->DebugTransform(transform_);
        ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);
        ImGui::ColorEdit4("Color", &color_.x); 
        ui_->DebugMaterialOverrides(&environmentCoefficient_, &lightingModeOverride_, &useClampSamplerOverride_, &enableLightingOverride_, "##AmOverrides");
    }
    ImGui::End();
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
