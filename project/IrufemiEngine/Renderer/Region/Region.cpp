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
DescriptorPool* ModelRegion::srvPool_ = nullptr;
ModelManager* ModelRegion::modelManager_ = nullptr;

void ModelRegion::Initialize(
    Camera* camera,
    const std::string& objFilename) {
    assert(camera);
    camera_ = camera;

    assert(modelManager_ && "Region::Initialize: ModelManager is not set.");
    managedModel_ = modelManager_->GetModel(objFilename);

    assert(managedModel_ && managedModel_->cpuModel && "Region::Initialize: model load failed.");
    assert(!managedModel_->cpuModel->meshes.empty() && "Model has no mesh.");
    const auto& mesh = managedModel_->cpuModel->meshes.front();

    // インスタンス固有リソースの生成
    CreateMaterialResources(mesh);
    EnsureLightAndCamera();
    EnsureSharedTexture(mesh);
}

const GpuMesh* ModelRegion::GetGpuMesh() const {
    if (managedModel_ && !managedModel_->gpuMeshes.empty()) {
        return managedModel_->gpuMeshes.front().get();
    }
    return nullptr;
}

void ModelRegion::CreateMaterialResources(const ObjMesh& mesh) {
    // マテリアル
    materialResource_ = dx_->CreateBufferResource(sizeof(Material));
    Material* mat = nullptr;
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&mat));
    
    // ObjMaterial から Material へ必要なデータをコピー
    mat->color = mesh.material.color;
    mat->enableLighting = mesh.material.enableLighting;
    mat->uvTransform = mesh.material.uvTransform;
    mat->metallic = mesh.material.metallic;
    mat->roughness = mesh.material.roughness;
    mat->environmentCoefficient = 0.0f;
    // hasTexture は EnsureSharedTexture で設定するため、ここではパスの有無で仮設定
    mat->hasTexture = !mesh.material.textureFilePath.empty();
    mat->lightingMode = mesh.material.enableLighting ? 3 : 0; // ライティングモードをPBR(3)に設定

    if (mat->color.w <= 0.0f) { mat->color.w = 1.0f; }
}

void ModelRegion::EnsureLightAndCamera() {
    // 初期化済み
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

    instanceBuffer_ = dx_->CreateBufferResource(sizeInBytes);

    if (instancingSrvIndex_ == UINT32_MAX) {
        if (!srvPool_) {
            OutputDebugStringA("Region::CreateOrResizeInstanceBuffer: srvAllocator_ is null\n");
            return;
        }
        uint32_t idx = srvPool_->Allocate();
        if (idx == DescriptorPool::kInvalid) {
            OutputDebugStringA("Region::CreateOrResizeInstanceBuffer: SRV Allocate failed\n");
            return;
        }
        instancingSrvIndex_ = idx;
        instancingSrvCPU_ = srvPool_->GetCPUHandle(idx);
        instancingSrvGPU_ = srvPool_->GetGPUHandle(idx);
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Format = DXGI_FORMAT_UNKNOWN;
    srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Buffer.FirstElement = 0;
    srv.Buffer.NumElements = instanceCount;
    srv.Buffer.StructureByteStride = stride;
    srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    dx_->GetDevice()->CreateShaderResourceView(instanceBuffer_.Get(), &srv, instancingSrvCPU_);
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
        data.WVP = Math::Multiply(world, Math::Multiply(view, proj));

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

    // バッファ確保（全インスタンス分確保しておく）
    CreateOrResizeInstanceBuffer(totalCount);

    uint8_t* dst = nullptr;
    HRESULT hr = instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&dst));
    assert(SUCCEEDED(hr));
    std::memcpy(dst, temp.data(), sizeof(InstanceData) * visibleInstanceCount_);
    instanceBuffer_->Unmap(0, nullptr);

    instanceDirty_ = false;

    // カメラ行列を保存
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
}

void ModelRegion::Draw() {
    if (!GetGpuMesh() || GetGpuMesh()->vertexCount == 0 || instances_.empty()) { return; }

    // カメラの行列が変更されたかチェック
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &camera_->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &camera_->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (instanceDirty_ || cameraChanged) {
        BuildInstanceBuffer(true);
    }

    drawManager_->DrawModelRegion(this);
}