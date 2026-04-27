#include "Region.h"
#include <cassert>
#include <cstring>
#include <vector>
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/DirectX/DescriptorPool.h"
#include "Application/camera/Camera.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Resource/Model/ModelManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Math/Geometry/Frustum.h"
#include "Engine/Core/Shape/Sphere.h"
#include "Engine/Manager/DrawManager.h"
#include "Resource/Texture/TextureManager.h"
#include "Renderer/VertexData.h"
#include "Engine/Graphics/Data/Material.h"
#include "Engine/Graphics/Data/DirectionalLight.h"
#include "Engine/Graphics/Data/CameraForGPU.h"

DirectXCommon* ModelRegion::dx_ = nullptr;
TextureManager* ModelRegion::textureManager_ = nullptr;
DrawManager* ModelRegion::drawManager_ = nullptr;
ModelManager* ModelRegion::modelManager_ = nullptr;
DescriptorPool* ModelRegion::srvPool_ = nullptr;

ModelRegion::~ModelRegion() {
    if (srvPool_ && dx_) {
        for (auto& idx : instancingSrvIndex_) {
            if (idx != UINT32_MAX) {
                srvPool_->FreeAfterFence(idx, dx_->GetFenceValue());
                idx = UINT32_MAX;
            }
        }
    }
}

void ModelRegion::Initialize(
    Camera* camera,
    const std::string& objFilename) {
    assert(camera);
    camera_ = camera;

    assert(modelManager_ && "Region::Initialize: ModelManager is not set.");
    managedModel_ = modelManager_->GetModelAsync(objFilename);
    isResourcesInitialized_ = false;

    auto status = managedModel_->status.load();
    if (status == ManagedModel::LoadingStatus::Loaded && managedModel_->cpuModel) {
        InitializeResources();
    }
}

void ModelRegion::InitializeResources() {
    if (!managedModel_ || !managedModel_->cpuModel || managedModel_->cpuModel->meshes.empty()) {
        return;
    }
    const auto& mesh = managedModel_->cpuModel->meshes.front();

    // インスタンス固有リソースの生成
    CreateMaterialResources(mesh);
    EnsureSharedTexture(mesh);
    
    isResourcesInitialized_ = true;
}

const GpuMesh* ModelRegion::GetGpuMesh() const {
    if (managedModel_ && !managedModel_->gpuMeshes.empty()) {
        return managedModel_->gpuMeshes.front().get();
    }
    return nullptr;
}

void ModelRegion::CreateMaterialResources(const ObjMesh& mesh) {
    materialBuffer_.Initialize(dx_);
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        materialBuffer_[i]->color = mesh.material.color;
        materialBuffer_[i]->enableLighting = mesh.material.enableLighting;
        materialBuffer_[i]->uvTransform = mesh.material.uvTransform;
        materialBuffer_[i]->metallic = mesh.material.metallic;
        materialBuffer_[i]->roughness = mesh.material.roughness;
        materialBuffer_[i]->environmentCoefficient = 0.0f;
        materialBuffer_[i]->hasTexture = !mesh.material.textureFilePath.empty();
        materialBuffer_[i]->lightingMode = mesh.material.enableLighting ? 3 : 0;
        if (materialBuffer_[i]->color.w <= 0.0f) { materialBuffer_[i]->color.w = 1.0f; }
    }
}


void ModelRegion::EnsureSharedTexture(const ObjMesh& mesh) {
    if (!mesh.material.textureFilePath.empty()) {
        textureHandle_ = textureManager_->GetTextureHandle(mesh.material.textureFilePath);
    } else {
        textureHandle_ = textureManager_->GetWhiteTextureHandle();
    }
    assert(textureHandle_.ptr != 0 && "Texture SRV handle is invalid");
}

void ModelRegion::CreateOrResizeInstanceBuffer(uint32_t instanceCount) {
    const UINT stride = sizeof(InstanceData);
    const UINT sizeInBytes = std::max<UINT>(stride * instanceCount, stride);
    uint32_t frameIndex = dx_->GetFrameIndex();

    instanceBuffer_[frameIndex] = dx_->CreateBufferResource(sizeInBytes);

    if (instancingSrvIndex_[frameIndex] == UINT32_MAX) {
        assert(srvPool_);
        uint32_t idx = srvPool_->Allocate();
        if (idx == DescriptorPool::kInvalid) { OutputDebugStringA("ModelRegion SRV allocate failed\n"); return; }
        instancingSrvIndex_[frameIndex] = idx;
        instancingSrvCPU_[frameIndex] = srvPool_->GetCPUHandle(idx);
        instancingSrvGPU_[frameIndex] = srvPool_->GetGPUHandle(idx);
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Format = DXGI_FORMAT_UNKNOWN;
    srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Buffer.FirstElement = 0;
    srv.Buffer.NumElements = instanceCount;
    srv.Buffer.StructureByteStride = stride;
    srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    dx_->GetDevice()->CreateShaderResourceView(instanceBuffer_[frameIndex].Get(), &srv, instancingSrvCPU_[frameIndex]);
}

void ModelRegion::AddInstance(const Transform& t) {
    instances_.push_back(t);
    instanceDirty_ = true;
}

void ModelRegion::ClearInstances() {
    instances_.clear();
    instanceDirty_ = true;
}

void ModelRegion::BuildInstanceBuffer(bool force) {
    if (!isResourcesInitialized_) {
        if (managedModel_->status.load() == ManagedModel::LoadingStatus::Loaded && managedModel_->cpuModel) {
            InitializeResources();
        } else {
            visibleInstanceCount_ = 0;
            return;
        }
    }

    if (instances_.empty()) { 
        visibleInstanceCount_ = 0;
        return; 
    }
    if (!force && !instanceDirty_) { return; }

    const UINT totalCount = static_cast<UINT>(instances_.size());
    
    // フィルタリング後のデータを格納する一時ベクタ
    std::vector<InstanceData> temp;
    temp.reserve(totalCount);

    const Matrix4x4 view = camera_->GetViewMatrix();
    const Matrix4x4 proj = camera_->GetPerspectiveFovMatrix();
    const Frustum& frustum = camera_->GetFrustum();
    float modelRadius = managedModel_ ? managedModel_->cpuModel->boundingSphere.radius : 0.0f;

    for (const auto& inst : instances_) {
        // 視錐台カリング
        if (isCullingEnabled_ && camera_) {
            float maxScale = (std::max)({ inst.scale.x, inst.scale.y, inst.scale.z });
            Sphere boundingSphere = managedModel_->cpuModel->boundingSphere;
            boundingSphere.center = inst.translate;
            boundingSphere.radius = modelRadius * maxScale * 1.1f;

            if (!Collision::IsCollision(frustum, boundingSphere)) {
                continue; // 画面外ならスキップ
            }
        }

        InstanceData data;
        Matrix4x4 world = Math::MakeAffineMatrix(inst.scale, inst.rotate, inst.translate);
        // data.WVP はシェーダーで使わなくなったため計算を省略
        data.WVP = Math::MakeIdentity4x4();

        Matrix4x4 worldForNormal = world;
        worldForNormal.m[3][0] = 0.0f;
        worldForNormal.m[3][1] = 0.0f;
        worldForNormal.m[3][2] = 0.0f;
        worldForNormal.m[3][3] = 1.0f;

        data.World = world;
        data.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
        data.color = { 1,1,1,1 };
        
        temp.push_back(data);
    }

    visibleInstanceCount_ = static_cast<uint32_t>(temp.size());
    if (visibleInstanceCount_ == 0) {
        instanceDirty_ = false;
        return;
    }

    // インスタンスバッファ確保 / 更新
    CreateOrResizeInstanceBuffer(totalCount);

    uint32_t frameIndex = dx_->GetFrameIndex();
    lastUpdateFrameIndex_ = frameIndex;
    uint8_t* dst = nullptr;
    HRESULT hr = instanceBuffer_[frameIndex]->Map(0, nullptr, reinterpret_cast<void**>(&dst));
    assert(SUCCEEDED(hr));
    std::memcpy(dst, temp.data(), sizeof(InstanceData) * visibleInstanceCount_);
    instanceBuffer_[frameIndex]->Unmap(0, nullptr);

    instanceDirty_ = false;
}

void ModelRegion::SyncBeforeDraw() {
    if (instanceDirty_) {
        BuildInstanceBuffer(false);
    }
}

void ModelRegion::Draw() {
    if (!GetGpuMesh() || GetGpuMesh()->vertexCount == 0 || instances_.empty()) { return; }

    // 毎フレームインスタンスの WVP 等を更新する (マルチバッファなので常に更新)
    SyncBeforeDraw();

    drawManager_->DrawModelRegion(this);
}