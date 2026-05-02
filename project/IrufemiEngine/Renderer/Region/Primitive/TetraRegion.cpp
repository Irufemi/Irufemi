#include "TetraRegion.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Graphics/DirectX/DescriptorPool.h" // 追加
#include "Application/camera/Camera.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Math/Geometry/Frustum.h"
#include "Engine/Core/Shape/Sphere.h"

#include <array>
#include <algorithm>
#include <cstring>
#include <cmath>

DirectXCommon* TetraRegion::dx_ = nullptr;
TextureManager* TetraRegion::textureManager_ = nullptr;
DrawManager* TetraRegion::drawManager_ = nullptr;
DescriptorPool* TetraRegion::srvPool_ = nullptr; // 追加

TetraRegion::~TetraRegion() {
    if (srvPool_ && dx_) {
        for (auto& idx : instancingSrvIndex_) {
            if (idx != UINT32_MAX) {
                srvPool_->FreeAfterFence(idx, dx_->GetFenceValue());
                idx = UINT32_MAX;
            }
        }
    }
}

void TetraRegion::Initialize(Camera* camera, const std::string& textureName) {
    assert(dx_ && "Call TetraRegion::SetDirectXCommon first");
    assert(textureManager_ != nullptr);
    camera_ = camera;

    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
    BuildTetraMesh(vertices, indices);
    CreateMeshBuffers(vertices, indices);
    CreateMaterialResources();
    EnsureLightAndCamera();
    EnsureSharedTexture(textureName);

    meshDirty_ = false;
}

void TetraRegion::BuildTetraMesh(std::vector<VertexData>& outVertices, std::vector<uint32_t>& outIndices) {
    outVertices.clear();
    outIndices.clear();

    const float k = edgeLength_ / (2.0f * std::sqrt(2.0f));
    std::array<Vector3, 4> pos = {
        Vector3{  k,  k,  k },
        Vector3{ -k, -k,  k },
        Vector3{ -k,  k, -k },
        Vector3{  k, -k, -k }
    };
    std::array<Vector2, 4> uv = {
        Vector2{0.5f, 1.0f}, Vector2{0.0f, 0.0f},
        Vector2{1.0f, 0.0f}, Vector2{0.5f, 0.0f}
    };

    for (int i = 0; i < 4; ++i) {
        VertexData vd{};
        vd.position = { pos[i].x, pos[i].y, pos[i].z, 1.0f };
        vd.texcoord = uv[i];
        float len = std::sqrt(pos[i].x * pos[i].x + pos[i].y * pos[i].y + pos[i].z * pos[i].z);
        if (len > 1e-6f) { vd.normal = { pos[i].x / len, pos[i].y / len, pos[i].z / len }; } else { vd.normal = { 0.0f,1.0f,0.0f }; }
        outVertices.push_back(vd);
    }

    outIndices.insert(outIndices.end(), { 0, 1, 2 });
    outIndices.insert(outIndices.end(), { 0, 3, 1 });
    outIndices.insert(outIndices.end(), { 0, 2, 3 });
    outIndices.insert(outIndices.end(), { 1, 3, 2 });
}

void TetraRegion::CreateMeshBuffers(const std::vector<VertexData>& vertices, const std::vector<uint32_t>& indices) {
    vertexCount_ = static_cast<UINT>(vertices.size());
    indexCount_ = static_cast<UINT>(indices.size());
    const size_t vbSize = sizeof(VertexData) * vertices.size();
    const size_t ibSize = sizeof(uint32_t) * indices.size();

    vertexResource_ = dx_->CreateBufferResource(vbSize);
    vertexBufferView_ = {};
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(vbSize);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* vb = nullptr;
    HRESULT hr = vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vb));
    assert(SUCCEEDED(hr));
    std::memcpy(vb, vertices.data(), vbSize);
    vertexResource_->Unmap(0, nullptr);

    indexResource_ = dx_->CreateBufferResource(ibSize);
    indexBufferView_ = {};
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = static_cast<UINT>(ibSize);
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    uint32_t* ib = nullptr;
    hr = indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&ib));
    assert(SUCCEEDED(hr));
    std::memcpy(ib, indices.data(), ibSize);
    indexResource_->Unmap(0, nullptr);
}

void TetraRegion::CreateMaterialResources() {
    if (auto engine = dx_->GetEngine()) {
        if (materialCbIndex_ == static_cast<uint32_t>(-1)) {
            materialCbIndex_ = engine->GetMaterialBufferManager()->Allocate();
        }
    }
    cpuMaterialData_.color = { 1,1,1,1 };
    cpuMaterialData_.enableLighting = true;
    cpuMaterialData_.hasTexture = true; // 仮
    cpuMaterialData_.lightingMode = 3;
    cpuMaterialData_.uvTransform = Math::MakeIdentity4x4();
    cpuMaterialData_.metallic = 0.0f;
    cpuMaterialData_.roughness = 0.5f;
    cpuMaterialData_.environmentCoefficient = 0.0f;

    if (auto engine = dx_->GetEngine()) {
        for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
            engine->GetMaterialBufferManager()->Update(materialCbIndex_, cpuMaterialData_, i);
        }
    }
}

void TetraRegion::EnsureSharedTexture(const std::string& textureName) {
    if (!textureName.empty()) {
        textureHandle_ = textureManager_->GetTextureHandle(textureName);
    }
    if (textureHandle_.ptr == 0) {
        textureHandle_ = textureManager_->GetWhiteTextureHandle();
        cpuMaterialData_.hasTexture = (textureHandle_.ptr != 0);
        if (auto engine = dx_->GetEngine()) {
            for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
                engine->GetMaterialBufferManager()->Update(materialCbIndex_, cpuMaterialData_, i);
            }
        }
    }
    assert(textureHandle_.ptr != 0 && "Texture SRV handle is invalid");
}

void TetraRegion::EnsureLightAndCamera() {
    // Draw 時に camera を反映するだけ
}

void TetraRegion::CreateOrResizeInstanceBuffer(uint32_t instanceCount) {
    const UINT stride = sizeof(InstanceData);
    const UINT sizeInBytes = std::max<UINT>(stride * instanceCount, stride);
    uint32_t frameIndex = dx_->GetFrameIndex();

    instanceBuffer_[frameIndex] = dx_->CreateBufferResource(sizeInBytes);

    if (instancingSrvIndex_[frameIndex] == UINT32_MAX) {
        assert(srvPool_);
        uint32_t idx = srvPool_->Allocate();
        if (idx == DescriptorPool::kInvalid) { OutputDebugStringA("TetraRegion SRV allocate failed\n"); return; }
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

void TetraRegion::AddInstance(const Transform& t) {
    instances_.push_back(t);
    instanceColors_.push_back({ 1,1,1,1 }); // 既定は白
    instanceDirty_ = true;
}

void TetraRegion::AddInstance(const Transform& t, const Vector4& color) {
    instances_.push_back(t);
    instanceColors_.push_back(color);
    instanceDirty_ = true;
}

void TetraRegion::AddInstance(const Vector3& center, float scale, const Vector3& rotate) {
    Transform t{};
    t.translate = center;
    t.scale = { scale, scale, scale };
    t.rotate = rotate;
    AddInstance(t);
}

void TetraRegion::AddInstance(const Vector3& center, float scale, const Vector3& rotate, const Vector4& color) {
    Transform t{};
    t.translate = center;
    t.scale = { scale, scale, scale };
    t.rotate = rotate;
    AddInstance(t, color);
}

void TetraRegion::AddInstanceWorld(const Matrix4x4& world, const Vector4& color) {
    instanceWorlds_.push_back(world);
    instanceWorldColors_.push_back(color);
    instanceDirty_ = true;
}

void TetraRegion::ClearInstances() {
    instances_.clear();
    instanceColors_.clear();
    instanceWorlds_.clear();
    instanceWorldColors_.clear();
    instanceDirty_ = true;
}

void TetraRegion::BuildInstanceBuffer(bool force) {
    const bool useWorlds = !instanceWorlds_.empty();
    const uint32_t totalCount = static_cast<uint32_t>(useWorlds ? instanceWorlds_.size() : instances_.size());
    if (totalCount == 0) { 
        visibleInstanceCount_ = 0;
        return; 
    }
    if (!force && !instanceDirty_) { return; }

    const Matrix4x4 view = camera_->GetViewMatrix();
    const Matrix4x4 proj = camera_->GetPerspectiveFovMatrix();
    const Frustum& frustum = camera_->GetFrustum();
    const float baseRadius = GetModelVertexRadius();

    std::vector<InstanceData> temp;
    temp.reserve(totalCount);

    if (useWorlds) {
        for (uint32_t i = 0; i < totalCount; ++i) {
            const Matrix4x4& world = instanceWorlds_[i];
            
            if (isCullingEnabled_) {
                // 位置とスケールの抽出
                Vector3 pos = { world.m[3][0], world.m[3][1], world.m[3][2] };
                // 簡易的な均等スケール抽出
                float scale = std::sqrt(world.m[0][0]*world.m[0][0] + world.m[0][1]*world.m[0][1] + world.m[0][2]*world.m[0][2]);
                
                Sphere boundingSphere;
                boundingSphere.center = pos;
                boundingSphere.radius = baseRadius * scale * 1.1f;
                if (!Collision::IsCollision(frustum, boundingSphere)) continue;
            }

            InstanceData data;
            data.WVP = Math::MakeIdentity4x4();
            Matrix4x4 worldForNormal = world;
            worldForNormal.m[3][0] = worldForNormal.m[3][1] = worldForNormal.m[3][2] = 0.0f;
            worldForNormal.m[3][3] = 1.0f;
            data.World = world;
            data.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
            data.color = (i < instanceWorldColors_.size()) ? instanceWorldColors_[i] : Vector4{ 1,1,1,1 };
            temp.push_back(data);
        }
    } else {
        if (instanceColors_.size() != instances_.size()) {
            instanceColors_.resize(instances_.size(), Vector4{ 1,1,1,1 });
        }
        for (uint32_t i = 0; i < totalCount; ++i) {
            const Transform& inst = instances_[i];

            if (isCullingEnabled_) {
                float maxScale = (std::max)({ inst.scale.x, inst.scale.y, inst.scale.z });
                Sphere boundingSphere;
                boundingSphere.center = inst.translate;
                boundingSphere.radius = baseRadius * maxScale * 1.1f;
                if (!Collision::IsCollision(frustum, boundingSphere)) continue;
            }

            InstanceData data;
            Matrix4x4 world = Math::MakeAffineMatrix(inst.scale, inst.rotate, inst.translate);
            data.WVP = Math::MakeIdentity4x4();
            Matrix4x4 worldForNormal = world;
            worldForNormal.m[3][0] = worldForNormal.m[3][1] = worldForNormal.m[3][2] = 0.0f;
            worldForNormal.m[3][3] = 1.0f;
            data.World = world;
            data.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
            data.color = instanceColors_[i];
            temp.push_back(data);
        }
    }

    visibleInstanceCount_ = static_cast<uint32_t>(temp.size());
    if (visibleInstanceCount_ == 0) {
        instanceDirty_ = false;
        return;
    }

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

void TetraRegion::SyncBeforeDraw() {
    if (vertexCount_ == 0 || indexCount_ == 0 || instances_.empty()) { return; }
    BuildInstanceBuffer(true);
}

void TetraRegion::Draw() {
    if (vertexCount_ == 0 || indexCount_ == 0 || (instances_.empty() && instanceWorlds_.empty())) { return; }
    

    drawManager_->SubmitRegion(vertexBufferView_, indexBufferView_, GetMaterialVAddress(), textureHandle_, instancingSrvGPU_[dx_->GetFrameIndex()], indexCount_, visibleInstanceCount_, castShadows_);
}

// --- サイズ関連 ---
float TetraRegion::GetModelVertexRadius() const {
    return edgeLength_ * std::sqrt(6.0f) / 4.0f;
}
float TetraRegion::ComputeScaleFromVertexRadius(float worldVertexRadius) const {
    const float base = GetModelVertexRadius();
    return (base > 1e-6f) ? (worldVertexRadius / base) : 0.0f;
}
void TetraRegion::AddInstanceByVertexRadius(const Vector3& center, float worldVertexRadius, const Vector3& rotate) {
    float scale = ComputeScaleFromVertexRadius(worldVertexRadius);
    AddInstance(center, scale, rotate);
}
void TetraRegion::SetEdge(float edge) {
    if (edge <= 0.0f) return;
    if (std::fabs(edge - edgeLength_) < 1e-6f) return;
    edgeLength_ = edge;
    meshDirty_ = true;
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
    BuildTetraMesh(vertices, indices);
    CreateMeshBuffers(vertices, indices);
    meshDirty_ = false;
}

// --- 色設定API ---
void TetraRegion::SetColor(const Vector4& color) {
    cpuMaterialData_.color = color;
    if (auto engine = dx_->GetEngine()) {
        for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
            engine->GetMaterialBufferManager()->Update(materialCbIndex_, cpuMaterialData_, i);
        }
    }
}
void TetraRegion::SetInstanceColor(uint32_t index, const Vector4& color) {
    if (index >= instanceColors_.size()) {
        assert(false && "TetraRegion::SetInstanceColor: index out of range");
        return;
    }
    instanceColors_[index] = color;
    instanceDirty_ = true;
}
void TetraRegion::SetAllInstanceColor(const Vector4& color) {
    if (instanceColors_.empty()) { return; }
    std::fill(instanceColors_.begin(), instanceColors_.end(), color);
    instanceDirty_ = true;
}

// --- 追加: ヘッダで宣言した静的セッターの実体定義 ---
void TetraRegion::SetDirectXCommon(DirectXCommon* dx) {
    dx_ = dx;
}
void TetraRegion::SetTextureManager(TextureManager* tm) {
    textureManager_ = tm;
}
void TetraRegion::SetDrawManager(DrawManager* dm) {
    drawManager_ = dm;
}


D3D12_GPU_VIRTUAL_ADDRESS TetraRegion::GetMaterialVAddress() const {
    if (materialCbIndex_ == static_cast<uint32_t>(-1)) return 0;
    return dx_->GetEngine()->GetMaterialBufferManager()->GetGPUVirtualAddress(materialCbIndex_, dx_->GetFrameIndex());
}
