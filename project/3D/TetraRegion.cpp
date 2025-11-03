#include "TetraRegion.h"
#include "engine/directX/DirectXCommon.h"
#include "engine/DescriptorAllocator.h" // 追加
#include "camera/Camera.h"
#include "manager/TextureManager.h"
#include "manager/DrawManager.h"

#include <array>
#include <algorithm>
#include <cstring>
#include <cmath>

DirectXCommon* TetraRegion::dx_ = nullptr;
TextureManager* TetraRegion::textureManager_ = nullptr;
DrawManager* TetraRegion::drawManager_ = nullptr;
DescriptorAllocator* TetraRegion::srvAllocator_ = nullptr; // 追加

TetraRegion::~TetraRegion() {
    // SRV スロットを遅延解放で返す
    if (instancingSrvIndex_ != UINT32_MAX && srvAllocator_ && dx_) {
        srvAllocator_->FreeAfterFence(instancingSrvIndex_, dx_->GetFenceValue());
        instancingSrvIndex_ = UINT32_MAX;
        instancingSrvCPU_ = {};
        instancingSrvGPU_ = {};
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
        float len = std::sqrt(pos[i].x*pos[i].x + pos[i].y*pos[i].y + pos[i].z*pos[i].z);
        if (len > 1e-6f) { vd.normal = { pos[i].x/len, pos[i].y/len, pos[i].z/len }; }
        else { vd.normal = {0.0f,1.0f,0.0f}; }
        outVertices.push_back(vd);
    }

    outIndices.insert(outIndices.end(), { 0, 1, 2 });
    outIndices.insert(outIndices.end(), { 0, 3, 1 });
    outIndices.insert(outIndices.end(), { 0, 2, 3 });
    outIndices.insert(outIndices.end(), { 1, 3, 2 });
}

void TetraRegion::CreateMeshBuffers(const std::vector<VertexData>& vertices, const std::vector<uint32_t>& indices) {
    vertexCount_ = static_cast<UINT>(vertices.size());
    indexCount_  = static_cast<UINT>(indices.size());
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
    materialResource_ = dx_->CreateBufferResource(sizeof(Material));
    Material* mat = nullptr;
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&mat));
    mat->color = {1,1,1,1};
    mat->enableLighting = true;
    mat->hasTexture = true;
    mat->lightingMode = 2;
    mat->uvTransform = Math::MakeIdentity4x4();
    mat->shininess = 32.0f;

    directionalLightResource_ = dx_->CreateBufferResource(sizeof(DirectionalLight));
    DirectionalLight* dl = nullptr;
    directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&dl));
    dl->color = {1,1,1,1};
    dl->direction = {0,-1,0};
    dl->intensity = 1.0f;

    cameraResource_ = dx_->CreateBufferResource(sizeof(CameraForGPU));
    CameraForGPU* cam = nullptr;
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cam));
    cam->worldPosition = {0,0,0};
}

void TetraRegion::EnsureSharedTexture(const std::string& textureName) {
    if (!textureName.empty()) {
        textureHandle_ = textureManager_->GetTextureHandle(textureName);
    }
    if (textureHandle_.ptr == 0) {
        textureHandle_ = textureManager_->GetWhiteTextureHandle();
        Material* mat = nullptr;
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&mat));
        mat->hasTexture = (textureHandle_.ptr != 0);
    }
    assert(textureHandle_.ptr != 0 && "Texture SRV handle is invalid");
}

void TetraRegion::EnsureLightAndCamera() {
    // Draw 時に camera を反映するだけ
}

void TetraRegion::CreateOrResizeInstanceBuffer(uint32_t instanceCount) {
    const UINT stride = sizeof(InstanceData);
    const UINT sizeInBytes = std::max<UINT>(stride * instanceCount, stride);
    instanceBuffer_ = dx_->CreateBufferResource(sizeInBytes);

    if (instancingSrvIndex_ == UINT32_MAX) {
        if (!srvAllocator_) { OutputDebugStringA("TetraRegion: srvAllocator_ is null\n"); return; }
        uint32_t idx = srvAllocator_->Allocate();
        if (idx == DescriptorAllocator::kInvalid) { OutputDebugStringA("TetraRegion: SRV Allocate failed\n"); return; }
        instancingSrvIndex_ = idx;
        instancingSrvCPU_ = srvAllocator_->GetCPUHandle(idx);
        instancingSrvGPU_ = srvAllocator_->GetGPUHandle(idx);
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

void TetraRegion::AddInstance(const Transform& t) {
    instances_.push_back(t);
    instanceColors_.push_back({1,1,1,1}); // 既定は白
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
    const uint32_t count = static_cast<uint32_t>(useWorlds ? instanceWorlds_.size() : instances_.size());
    if (count == 0) { return; }
    if (!force && !instanceDirty_) { return; }

    CreateOrResizeInstanceBuffer(count);

    std::vector<InstanceData> temp(count);
    const Matrix4x4 view = camera_->GetViewMatrix();
    const Matrix4x4 proj = camera_->GetPerspectiveFovMatrix();

    if (useWorlds) {
        for (uint32_t i = 0; i < count; ++i) {
            const Matrix4x4& world = instanceWorlds_[i];
            Matrix4x4 wvp = Math::Multiply(world, Math::Multiply(view, proj));

            Matrix4x4 worldForNormal = world;
            worldForNormal.m[3][0] = 0.0f;
            worldForNormal.m[3][1] = 0.0f;
            worldForNormal.m[3][2] = 0.0f;
            worldForNormal.m[3][3] = 1.0f;

            temp[i].WVP = wvp;
            temp[i].World = world;
            temp[i].WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
            temp[i].color = (i < instanceWorldColors_.size()) ? instanceWorldColors_[i] : Vector4{1,1,1,1};
        }
    } else {
        // Transform系の色配列をサイズ同期
        if (instanceColors_.size() != instances_.size()) {
            instanceColors_.resize(instances_.size(), Vector4{1,1,1,1});
        }

        for (uint32_t i = 0; i < count; ++i) {
            const Transform& inst = instances_[i];
            Matrix4x4 world = Math::MakeAffineMatrix(inst.scale, inst.rotate, inst.translate);
            Matrix4x4 wvp = Math::Multiply(world, Math::Multiply(view, proj));

            Matrix4x4 worldForNormal = world;
            worldForNormal.m[3][0] = 0.0f;
            worldForNormal.m[3][1] = 0.0f;
            worldForNormal.m[3][2] = 0.0f;
            worldForNormal.m[3][3] = 1.0f;

            temp[i].WVP = wvp;
            temp[i].World = world;
            temp[i].WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
            temp[i].color = instanceColors_[i];
        }
    }

    uint8_t* dst = nullptr;
    HRESULT hr = instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&dst));
    assert(SUCCEEDED(hr));
    std::memcpy(dst, temp.data(), sizeof(InstanceData) * count);
    instanceBuffer_->Unmap(0, nullptr);

    instanceDirty_ = false;
}

void TetraRegion::Draw() {
    // 描画スキップ条件
    if ((vertexCount_ == 0 || indexCount_ == 0)) { return; }
    const bool useWorlds = !instanceWorlds_.empty();
    if (!useWorlds && instances_.empty()) { return; }

    // カメラワールド位置更新
    CameraForGPU* cam = nullptr;
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cam));
    cam->worldPosition = camera_->GetTranslate();

    // インスタンス更新
    BuildInstanceBuffer(true);

    drawManager_->DrawTetraRegion(this);
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
    Material* mat = nullptr;
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&mat));
    mat->color = color;
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
