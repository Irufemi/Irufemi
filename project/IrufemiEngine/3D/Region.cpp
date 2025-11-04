#include "Region.h"

#include <cassert>
#include <cstring>
#include "engine/directX/DirectXCommon.h"
#include "engine/DescriptorAllocator.h" // 追加
#include "camera/Camera.h"
#include "manager/TextureManager.h"
#include "manager/DrawManager.h"
#include "function/Function.h" // LoadObjFileM, 型定義
#include "function/Math.h"
#include "math/Transform.h"
#include "math/Material.h"   // Material
#include "math/DirectionalLight.h"      // DirectionalLight
#include "math/CameraForGPU.h"

DirectXCommon* Region::dx_ = nullptr;
TextureManager* Region::textureManager_ = nullptr;
DrawManager* Region::drawManager_ = nullptr;
DescriptorAllocator* Region::srvAllocator_ = nullptr; // 追加

void Region::Initialize(
    Camera* camera,
    const std::string& objFilename) {
    assert(camera);
    camera_ = camera;

    // OBJ 読み込み（単一メッシュ前提）
    objModel_ = LoadObjFileM("resources/obj", objFilename);
    assert(!objModel_.meshes.empty() && "objModel has no mesh");
    const auto& mesh = objModel_.meshes.front();

    // メッシュの VB 作成
    CreateMeshBuffers(mesh);

    // マテリアル/ライト/カメラ
    CreateMaterialResources(mesh);
    EnsureLightAndCamera();

    // テクスチャ共有（SRV 再利用）
    EnsureSharedTexture(mesh);

    // インスタンシングバッファは必要になった時に作成（最低1で良ければここで CreateOrResizeInstanceBuffer(1) でもOK）
}

void Region::CreateMeshBuffers(const ObjMesh& mesh) {
    vertexCount_ = static_cast<UINT>(mesh.vertices.size());
    const size_t vbSize = sizeof(VertexData) * mesh.vertices.size();

    vertexResource_ = dx_->CreateBufferResource(vbSize);
    vertexBufferView_ = D3D12_VERTEX_BUFFER_VIEW{};
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(vbSize);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // 転送
    VertexData* vb = nullptr;
    HRESULT hr = vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vb));
    assert(SUCCEEDED(hr));
    std::memcpy(vb, mesh.vertices.data(), vbSize);
    vertexResource_->Unmap(0, nullptr);
}

void Region::CreateMaterialResources(const ObjMesh& mesh) {
    // マテリアル
    materialResource_ = dx_->CreateBufferResource(sizeof(Material));
    Material* mat = nullptr;
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&mat));
    mat->color = mesh.material.color;
    mat->enableLighting = mesh.material.enableLighting;
    mat->hasTexture = !mesh.material.textureFilePath.empty();
    mat->lightingMode = mesh.material.enableLighting ? 2 : 0;
    mat->uvTransform = mesh.material.uvTransform;
    mat->shininess = mesh.material.shininess;

    // ライト
    directionalLightResource_ = dx_->CreateBufferResource(sizeof(DirectionalLight));
    DirectionalLight* dl = nullptr;
    directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&dl));
    dl->color = { 1.0f,1.0f,1.0f,1.0f };
    dl->direction = { 0.0f,-1.0f,0.0f };
    dl->intensity = 1.0f;

    // カメラ
    cameraResource_ = dx_->CreateBufferResource(sizeof(CameraForGPU));
    CameraForGPU* cam = nullptr;
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cam));
    cam->worldPosition = camera_->GetTranslate();
}

void Region::EnsureLightAndCamera() {
    // 初期化済み。毎フレームのカメラ位置更新は Draw 内で行う
}

void Region::EnsureSharedTexture(const ObjMesh& mesh) {
    if (!mesh.material.textureFilePath.empty()) {
        textureHandle_ = textureManager_->GetTextureHandle(mesh.material.textureFilePath);
    } else {
        textureHandle_ = textureManager_->GetWhiteTextureHandle();
    }
    assert(textureHandle_.ptr != 0 && "Texture SRV handle is invalid");
}

void Region::CreateOrResizeInstanceBuffer(uint32_t instanceCount) {
    const UINT stride = sizeof(InstanceData);
    const UINT sizeInBytes = std::max<UINT>(stride * instanceCount, stride);

    instanceBuffer_ = dx_->CreateBufferResource(sizeInBytes);

    // 初回のみ SRV スロット確保（以降は上書き再利用）
    if (instancingSrvIndex_ == UINT32_MAX) {
        if (!srvAllocator_) {
            OutputDebugStringA("Region::CreateOrResizeInstanceBuffer: srvAllocator_ is null\n");
            return;
        }
        uint32_t idx = srvAllocator_->Allocate();
        if (idx == DescriptorAllocator::kInvalid) {
            OutputDebugStringA("Region::CreateOrResizeInstanceBuffer: SRV Allocate failed\n");
            return;
        }
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

    // 同一ハンドルに上書き（再利用）
    dx_->GetDevice()->CreateShaderResourceView(instanceBuffer_.Get(), &srv, instancingSrvCPU_);
}

void Region::AddInstance(const Transform& t) {
    instances_.push_back(t);
    instanceDirty_ = true;
}

void Region::ClearInstances() {
    instances_.clear();
    instanceDirty_ = true;
}

// 変更: force を見るように
void Region::BuildInstanceBuffer(bool force) {
    if (instances_.empty()) { return; }
    if (!force && !instanceDirty_) { return; }

    const UINT count = static_cast<UINT>(instances_.size());
    const UINT stride = sizeof(InstanceData);
    const UINT sizeInBytes = stride * count;

    // バッファ確保・SRV更新（要素数が変わったときだけ作り直ししたい場合は、既存サイズを保持して条件分岐）
    CreateOrResizeInstanceBuffer(count);

    std::vector<InstanceData> temp(count);
    const Matrix4x4 view = camera_->GetViewMatrix();
    const Matrix4x4 proj = camera_->GetPerspectiveFovMatrix();
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
        temp[i].color = { 1,1,1,1 };
    }

    uint8_t* dst = nullptr;
    HRESULT hr = instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&dst));
    assert(SUCCEEDED(hr));
    std::memcpy(dst, temp.data(), sizeInBytes);
    instanceBuffer_->Unmap(0, nullptr);

    instanceDirty_ = false;
}

void Region::Draw() {
    if (vertexCount_ == 0 || instances_.empty()) { return; }

    // カメラ位置更新
    {
        CameraForGPU* cam = nullptr;
        cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cam));
        cam->worldPosition = camera_->GetTranslate();
    }

    // インスタンスバッファ更新（毎フレームWVP再計算）
    BuildInstanceBuffer(true);

    drawManager_->DrawRegion(this);

}