#include "SphereRegion.h"
#include "engine/directX/DirectXCommon.h"
#include "camera/Camera.h"
#include "manager/TextureManager.h"
#include "manager/DrawManager.h"
#include "engine/DescriptorAllocator.h" // 追加

DirectXCommon* SphereRegion::dx_ = nullptr;
TextureManager* SphereRegion::textureManager_ = nullptr;
DrawManager* SphereRegion::drawManager_ = nullptr;
DescriptorAllocator* SphereRegion::srvAllocator_ = nullptr; // 追加

SphereRegion::~SphereRegion() {
    // SRV スロットを遅延解放で返す
    if (instancingSrvIndex_ != UINT32_MAX && srvAllocator_ && dx_) {
        srvAllocator_->FreeAfterFence(instancingSrvIndex_, dx_->GetFenceValue());
        instancingSrvIndex_ = UINT32_MAX;
        instancingSrvCPU_ = {};
        instancingSrvGPU_ = {};
    }
}

void SphereRegion::Initialize(Camera* camera, const std::string& textureName, uint32_t subdivision) {
    assert(dx_ && "Call SphereRegion::SetDirectXCommon first");
    assert(textureManager_ && "Call SphereRegion::SetTextureManager first");
    assert(camera);
    camera_ = camera;

    // スフィアメッシュ生成（単位球）
    std::vector<VertexData> vertices;
    std::vector<uint32_t>   indices;
    BuildSphereMesh(subdivision, vertices, indices);

    // メッシュVB/IB
    CreateMeshBuffers(vertices, indices);

    // マテリアル/ライト/カメラ
    CreateMaterialResources();
    EnsureLightAndCamera();

    // テクスチャ共有（SRV再利用）
    EnsureSharedTexture(textureName);
}

void SphereRegion::BuildSphereMesh(uint32_t subdivision, std::vector<VertexData>& outVertices, std::vector<uint32_t>& outIndices) {
    outVertices.clear();
    outIndices.clear();

    const float pi = 3.141592654f;
    const float kLonEvery = 2.0f * pi / static_cast<float>(subdivision);
    const float kLatEvery = pi / static_cast<float>(subdivision);

    // 頂点生成（上から下へ 緯度×経度グリッド）
    for (uint32_t lat = 0; lat <= subdivision; ++lat) {
        float theta = -pi / 2.0f + kLatEvery * lat; // θ
        for (uint32_t lon = 0; lon <= subdivision; ++lon) {
            float phi = kLonEvery * lon; // φ

            Vector3 p{
                std::cos(theta) * std::cos(phi),
                std::sin(theta),
                std::cos(theta) * std::sin(phi)
            };
            Vector2 uv{
                static_cast<float>(lon) / static_cast<float>(subdivision),
                1.0f - static_cast<float>(lat) / static_cast<float>(subdivision)
            };

            VertexData v{};
            v.position = { p.x, p.y, p.z, 1.0f };
            v.texcoord = uv;
            v.normal = { p.x, p.y, p.z }; // 単位球なので位置＝法線
            outVertices.push_back(v);
        }
    }

    // インデックス（各クワッドを2トライアングル）
    const uint32_t stride = subdivision + 1;
    for (uint32_t lat = 0; lat < subdivision; ++lat) {
        for (uint32_t lon = 0; lon < subdivision; ++lon) {
            uint32_t a = stride * lat + lon;
            uint32_t b = stride * (lat + 1) + lon;
            uint32_t c = a + 1;
            uint32_t d = b + 1;

            outIndices.push_back(a);
            outIndices.push_back(b);
            outIndices.push_back(c);

            outIndices.push_back(b);
            outIndices.push_back(d);
            outIndices.push_back(c);
        }
    }
}

void SphereRegion::CreateMeshBuffers(const std::vector<VertexData>& vertices, const std::vector<uint32_t>& indices) {
    vertexCount_ = static_cast<UINT>(vertices.size());
    indexCount_ = static_cast<UINT>(indices.size());
    const size_t vbSize = sizeof(VertexData) * vertices.size();
    const size_t ibSize = sizeof(uint32_t) * indices.size();

    // VB
    vertexResource_ = dx_->CreateBufferResource(vbSize);
    vertexBufferView_ = D3D12_VERTEX_BUFFER_VIEW{};
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(vbSize);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* vb = nullptr;
    HRESULT hr = vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vb));
    assert(SUCCEEDED(hr));
    std::memcpy(vb, vertices.data(), vbSize);
    vertexResource_->Unmap(0, nullptr);

    // IB
    indexResource_ = dx_->CreateBufferResource(ibSize);
    indexBufferView_ = D3D12_INDEX_BUFFER_VIEW{};
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = static_cast<UINT>(ibSize);
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    uint32_t* ib = nullptr;
    hr = indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&ib));
    assert(SUCCEEDED(hr));
    std::memcpy(ib, indices.data(), ibSize);
    indexResource_->Unmap(0, nullptr);
}

void SphereRegion::CreateMaterialResources() {
    // Material
    materialResource_ = dx_->CreateBufferResource(sizeof(Material));
    Material* mat = nullptr;
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&mat));
    mat->color = { 1,1,1,1 };
    mat->enableLighting = true;
    mat->hasTexture = true; // 実際の有無は EnsureSharedTexture で調整
    mat->lightingMode = 2;
    mat->uvTransform = Math::MakeIdentity4x4();
    mat->shininess = 64.0f;

    // Light
    directionalLightResource_ = dx_->CreateBufferResource(sizeof(DirectionalLight));
    DirectionalLight* dl = nullptr;
    directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&dl));
    dl->color = { 1,1,1,1 };
    dl->direction = { 0,-1,0 };
    dl->intensity = 1.0f;

    // Camera
    cameraResource_ = dx_->CreateBufferResource(sizeof(CameraForGPU));
    CameraForGPU* cam = nullptr;
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cam));
    cam->worldPosition = { 0,0,0 };
}

void SphereRegion::EnsureSharedTexture(const std::string& textureName) {
    if (!textureName.empty()) {
        textureHandle_ = textureManager_->GetTextureHandle(textureName);
    }
    if (textureHandle_.ptr == 0) {
        textureHandle_ = textureManager_->GetWhiteTextureHandle();
        // マテリアル側の hasTexture は実際のSRV存在に合わせて PS で参照
        Material* mat = nullptr;
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&mat));
        mat->hasTexture = (textureHandle_.ptr != 0);
    }
    assert(textureHandle_.ptr != 0 && "Texture SRV handle is invalid");
}

void SphereRegion::EnsureLightAndCamera() {
    // 追加処理は不要。Draw 時にカメラ位置を更新
}

void SphereRegion::CreateOrResizeInstanceBuffer(uint32_t instanceCount) {
    const UINT stride = sizeof(InstanceData);
    const UINT sizeInBytes = std::max<UINT>(stride * instanceCount, stride);

    instanceBuffer_ = dx_->CreateBufferResource(sizeInBytes);

    if (instancingSrvIndex_ == UINT32_MAX) {
        assert(srvAllocator_ && "SphereRegion::SetSrvAllocator 未設定");
        uint32_t idx = srvAllocator_->Allocate();
        if (idx == DescriptorAllocator::kInvalid) { OutputDebugStringA("SphereRegion: SRV Allocate failed\n"); return; }
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

    // 同じハンドルに上書き（再利用）
    dx_->GetDevice()->CreateShaderResourceView(instanceBuffer_.Get(), &srv, instancingSrvCPU_);
}

void SphereRegion::AddInstance(const Transform& t) {
    instances_.push_back(t);
    instanceColors_.push_back({1,1,1,1}); // 既定は白
    instanceDirty_ = true;
}

void SphereRegion::AddInstance(const Transform& t, const Vector4& color) {
    instances_.push_back(t);
    instanceColors_.push_back(color);
    instanceDirty_ = true;
}

void SphereRegion::AddInstance(const Vector3& center, float radius, const Vector3& rotate) {
    Transform t{};
    t.scale = { radius, radius, radius };
    t.rotate = rotate;
    t.translate = center;
    AddInstance(t);
}

void SphereRegion::AddInstance(const Vector3& center, float radius, const Vector3& rotate, const Vector4& color) {
    Transform t{};
    t.scale = { radius, radius, radius };
    t.rotate = rotate;
    t.translate = center;
    AddInstance(t, color);
}

void SphereRegion::ClearInstances() {
    instances_.clear();
    instanceColors_.clear();
    instanceDirty_ = true;
}

void SphereRegion::BuildInstanceBuffer(bool force) {
    if (instances_.empty()) { return; }
    if (!force && !instanceDirty_) { return; }

    const UINT count = static_cast<UINT>(instances_.size());
    const UINT stride = sizeof(InstanceData);
    const UINT sizeInBytes = stride * count;

    // 今回は毎回作り直し（必要ならサイズ比較して再利用可）
    CreateOrResizeInstanceBuffer(count);

    std::vector<InstanceData> temp(count);

    const Matrix4x4 view = camera_->GetViewMatrix();
    const Matrix4x4 proj = camera_->GetPerspectiveFovMatrix();

    // 色配列サイズをインスタンス数に合わせる
    if (instanceColors_.size() != instances_.size()) {
        instanceColors_.resize(instances_.size(), {1,1,1,1});
    }

    for (UINT i = 0; i < count; ++i) {
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

    uint8_t* dst = nullptr;
    HRESULT hr = instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&dst));
    assert(SUCCEEDED(hr));
    std::memcpy(dst, temp.data(), sizeInBytes);
    instanceBuffer_->Unmap(0, nullptr);

    instanceDirty_ = false;
}

void SphereRegion::Draw() {
    if (vertexCount_ == 0 || indexCount_ == 0 || instances_.empty()) { return; }

    // カメラワールド位置更新
    {
        CameraForGPU* cam = nullptr;
        cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cam));
        cam->worldPosition = camera_->GetTranslate();
    }

    // 毎フレームインスタンスの WVP 更新
    BuildInstanceBuffer(true);

    drawManager_->DrawSphereRegion(this);

}

void SphereRegion::SetColor(const Vector4& color) {
    // マテリアル色（テクスチャと乗算される想定）
    Material* mat = nullptr;
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&mat));
    mat->color = color;
}

void SphereRegion::SetInstanceColor(uint32_t index, const Vector4& color) {
    if (index >= instanceColors_.size()) {
        assert(false && "SphereRegion::SetInstanceColor: index out of range");
        return;
    }
    instanceColors_[index] = color;
    instanceDirty_ = true;
}

void SphereRegion::SetAllInstanceColor(const Vector4& color) {
    if (instanceColors_.empty()) { return; }
    std::fill(instanceColors_.begin(), instanceColors_.end(), color);
    instanceDirty_ = true;
}