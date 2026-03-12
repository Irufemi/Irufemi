#include "GPUParticleSystem.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/DirectX/DescriptorPool.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Core/Math/Geometry/Math.h"
#include "Resource/Texture/TextureManager.h"
#include "Renderer/VertexData.h"
#include "Application/camera/Camera.h"
#include <cassert>

// 静的メンバ変数の実体定義
DirectXCommon* GPUParticleSystem::dxCommon_ = nullptr;
DrawManager* GPUParticleSystem::drawManager_ = nullptr;
TextureManager* GPUParticleSystem::textureManager_ = nullptr;
IrufemiEngine* GPUParticleSystem::engine_ = nullptr;

// コンストラクタ
GPUParticleSystem::GPUParticleSystem() = default;

// デストラクタ
GPUParticleSystem::~GPUParticleSystem() = default;

// 初期化
void GPUParticleSystem::Initialize(Camera* camera, const std::string& textureName) {

    assert(dxCommon_);
    assert(drawManager_);
    assert(textureManager_);
    assert(engine_);

    camera_ = camera;

    auto* srvPool = dxCommon_->GetSrvPool();

    /*EmitterSphere*/
    emitterSphereResource_ = dxCommon_->CreateBufferResource(sizeof(EmitterSphere));
    emitterSphereResource_->Map(0, nullptr, reinterpret_cast<void**>(&emitterSphere_));

    perFrameResource_ = dxCommon_->CreateBufferResource(sizeof(PerFrame));
    perFrameResource_->Map(0, nullptr, reinterpret_cast<void**>(&perFrameData_));

    // SRV
    uint32_t emitterSrvIndex = srvPool->Allocate();
    emitterSphereSrvHandleCPU_ = srvPool->GetCPUHandle(emitterSrvIndex);
    emitterSphereSrvHandleGPU_ = srvPool->GetGPUHandle(emitterSrvIndex);
    D3D12_SHADER_RESOURCE_VIEW_DESC emitterSrvDesc{};
    emitterSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    emitterSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    emitterSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    emitterSrvDesc.Buffer.FirstElement = 0;
    emitterSrvDesc.Buffer.NumElements = 1;
    emitterSrvDesc.Buffer.StructureByteStride = sizeof(EmitterSphere);
    dxCommon_->GetDevice()->CreateShaderResourceView(emitterSphereResource_.Get(), &emitterSrvDesc, emitterSphereSrvHandleCPU_);

    // perFrame SRV
    uint32_t perFrameSrvIndex = srvPool->Allocate();
    perFrameSrvHandleCPU_ = srvPool->GetCPUHandle(perFrameSrvIndex);
    perFrameSrvHandleGPU_ = srvPool->GetGPUHandle(perFrameSrvIndex);
    D3D12_SHADER_RESOURCE_VIEW_DESC perFrameSrvDesc{};
    perFrameSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    perFrameSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    perFrameSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    perFrameSrvDesc.Buffer.FirstElement = 0;
    perFrameSrvDesc.Buffer.NumElements = 1;
    perFrameSrvDesc.Buffer.StructureByteStride = sizeof(PerFrame);
    dxCommon_->GetDevice()->CreateShaderResourceView(perFrameResource_.Get(), &perFrameSrvDesc, perFrameSrvHandleCPU_);


    /*GPUParticle*/

    // 1. Particleの情報を格納するためのResourceをD3D12_HEAP_TYPE_DEFAULTで作る
    particleResource_ = dxCommon_->CreateUAVBufferResource(sizeof(ParticleCS) * kMaxParticles);

    // 2. 1に対してUAV等のViewを作る
    // UAV
    uint32_t uavIndex = srvPool->Allocate();
    particleUavHandleCPU_ = srvPool->GetCPUHandle(uavIndex);
    particleUavHandleGPU_ = srvPool->GetGPUHandle(uavIndex);
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = kMaxParticles;
    uavDesc.Buffer.StructureByteStride = sizeof(ParticleCS);
    dxCommon_->GetDevice()->CreateUnorderedAccessView(particleResource_.Get(), nullptr, &uavDesc, particleUavHandleCPU_);

    // SRV
    uint32_t srvIndex = srvPool->Allocate();
    particleSrvHandleCPU_ = srvPool->GetCPUHandle(srvIndex);
    particleSrvHandleGPU_ = srvPool->GetGPUHandle(srvIndex);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = kMaxParticles;
    srvDesc.Buffer.StructureByteStride = sizeof(ParticleCS);
    dxCommon_->GetDevice()->CreateShaderResourceView(particleResource_.Get(), &srvDesc, particleSrvHandleCPU_);

    // freeListIndexリソース
    freeListIndexResource_ = dxCommon_->CreateUAVBufferResource(sizeof(int32_t));
    // UAV
    uint32_t freeListIndexUavIndex = srvPool->Allocate();
    freeListIndexUavHandleCPU_ = srvPool->GetCPUHandle(freeListIndexUavIndex);
    freeListIndexUavHandleGPU_ = srvPool->GetGPUHandle(freeListIndexUavIndex);
    D3D12_UNORDERED_ACCESS_VIEW_DESC freeListIndexUavDesc{};
    freeListIndexUavDesc.Format = DXGI_FORMAT_UNKNOWN;
    freeListIndexUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    freeListIndexUavDesc.Buffer.FirstElement = 0;
    freeListIndexUavDesc.Buffer.NumElements = 1;
    freeListIndexUavDesc.Buffer.StructureByteStride = sizeof(int32_t);
    dxCommon_->GetDevice()->CreateUnorderedAccessView(freeListIndexResource_.Get(), nullptr, &freeListIndexUavDesc, freeListIndexUavHandleCPU_);

    // freeListリソース
    freeListResource_ = dxCommon_->CreateUAVBufferResource(sizeof(int32_t) * kMaxParticles);
    // UAV
    uint32_t freeListUavIndex = srvPool->Allocate();
    freeListUavHandleCPU_ = srvPool->GetCPUHandle(freeListUavIndex);
    freeListUavHandleGPU_ = srvPool->GetGPUHandle(freeListUavIndex);
    D3D12_UNORDERED_ACCESS_VIEW_DESC freeListUavDesc{};
    freeListUavDesc.Format = DXGI_FORMAT_UNKNOWN;
    freeListUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    freeListUavDesc.Buffer.FirstElement = 0;
    freeListUavDesc.Buffer.NumElements = kMaxParticles;
    freeListUavDesc.Buffer.StructureByteStride = sizeof(int32_t);
    dxCommon_->GetDevice()->CreateUnorderedAccessView(freeListResource_.Get(), nullptr, &freeListUavDesc, freeListUavHandleCPU_);

    // PerView用リソース
    perViewResource_ = dxCommon_->CreateBufferResource(sizeof(PerViewForGPU));
    perViewResource_->Map(0, nullptr, reinterpret_cast<void**>(&perViewData_));

    // Material用リソース
    materialResource_ = dxCommon_->CreateBufferResource(sizeof(ParticleGPUMaterial));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->uvTransform = Math::MakeIdentity4x4();
    materialData_->useClampSampler = 0;

    // 頂点バッファ
    vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * 6);
    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    // 四角ポリゴン
    vertexData[0].position = { -1.0f, 1.0f, 0.0f, 1.0f };
    vertexData[0].texcoord = { 0.0f, 0.0f };
    vertexData[1].position = { 1.0f, 1.0f, 0.0f, 1.0f };
    vertexData[1].texcoord = { 1.0f, 0.0f };
    vertexData[2].position = { -1.0f, -1.0f, 0.0f, 1.0f };
    vertexData[2].texcoord = { 0.0f, 1.0f };
    vertexData[3].position = { -1.0f, -1.0f, 0.0f, 1.0f };
    vertexData[3].texcoord = { 0.0f, 1.0f };
    vertexData[4].position = { 1.0f, 1.0f, 0.0f, 1.0f };
    vertexData[4].texcoord = { 1.0f, 0.0f };
    vertexData[5].position = { 1.0f, -1.0f, 0.0f, 1.0f };
    vertexData[5].texcoord = { 1.0f, 1.0f };
    vertexResource_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * 6;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    textureHandle_ = textureManager_->GetTextureHandle(textureName);

    // 3. 1のResourceに対する初期化処理をCSで行う
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // ディスクリプタヒープを設定
    ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetComputeRootSignature(dxCommon_->GetComputeRootSignature());
    commandList->SetPipelineState(dxCommon_->GetGpuParticleInitializePSO());
    commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_);
    commandList->SetComputeRootDescriptorTable(6, freeListIndexUavHandleGPU_);
    commandList->SetComputeRootDescriptorTable(7, freeListUavHandleGPU_);

    commandList->Dispatch((kMaxParticles + 1023) / 1024, 1, 1);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = nullptr; // グローバルUAVバリア
    commandList->ResourceBarrier(1, &barrier);


    /*Particleを発生させる*/

    emitterSphere_->count = 10;
    emitterSphere_->frequency = 0.5f;
    emitterSphere_->frequencyTime = 0.0f;
    emitterSphere_->translate = Vector3(0.0f, 0.0f, 0.0f);
    emitterSphere_->radius = 1.0f;
    emitterSphere_->emit = 1;

    perFrameData_->deltaTime = engine_->GetDeltaTime();

}

// 更新
void GPUParticleSystem::Update() {

    /*Particleを発生させる*/

    emitterSphere_->frequencyTime += engine_->GetDeltaTime(); // δタイムを加算
    perFrameData_->time = engine_->GetTotalTime();
    perFrameData_->deltaTime = engine_->GetDeltaTime();

    // 射出間隔を上回ったら射出許可を出して時間を調整
    if (emitterSphere_->frequency <= emitterSphere_->frequencyTime) {
        emitterSphere_->frequencyTime -= emitterSphere_->frequency;
        emitterSphere_->emit = 1;
    // 射出間隔を上回っていないので、射出許可は出せない
    } else {
        emitterSphere_->emit = 0;
    }

    /*GPUParticle*/

    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // ディスクリプタヒープを設定
    ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetComputeRootSignature(dxCommon_->GetComputeRootSignature());

    // UAVバリアの設定
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = nullptr; // グローバルUAVバリア

    // Emit
    commandList->SetPipelineState(dxCommon_->GetGpuParticleEmitPSO());
    commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_);
    commandList->SetComputeRootDescriptorTable(6, freeListIndexUavHandleGPU_);
    commandList->SetComputeRootDescriptorTable(7, freeListUavHandleGPU_);
    commandList->SetComputeRootConstantBufferView(4, emitterSphereResource_->GetGPUVirtualAddress());
    commandList->SetComputeRootConstantBufferView(5, perFrameResource_->GetGPUVirtualAddress());
    commandList->Dispatch(1, 1, 1);

    // Emitの完了を待つ
    commandList->ResourceBarrier(1, &barrier);

    // Update
    commandList->SetPipelineState(dxCommon_->GetGpuParticleUpdatePSO());
    commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_);
    commandList->SetComputeRootDescriptorTable(6, freeListIndexUavHandleGPU_);
    commandList->SetComputeRootDescriptorTable(7, freeListUavHandleGPU_);
    commandList->SetComputeRootConstantBufferView(5, perFrameResource_->GetGPUVirtualAddress());
    commandList->Dispatch((kMaxParticles + 1023) / 1024, 1, 1);

    // Updateの完了を待つ
    commandList->ResourceBarrier(1, &barrier);


    perViewData_->viewProjection = camera_->GetViewProjectionMatrix3D();

    // backToFrontMatrix_の設定(面の向きをカメラの方向にしてあるのでここは調整なし。0でOK)
    Matrix4x4 backToFrontMatrix_ = Math::MakeRotateYMatrix(0.0f);

    /// カメラの回転を適用する
    Matrix4x4 billboardMatrix_ = Math::Multiply(backToFrontMatrix_, camera_->GetCameraMatrix());
    billboardMatrix_.m[3][0] = 0.0f;
    billboardMatrix_.m[3][1] = 0.0f;
    billboardMatrix_.m[3][2] = 0.0f;

    perViewData_->billbordMatrix = billboardMatrix_;
}

// 描画
void GPUParticleSystem::Draw() {

    /*GPUParticle*/

    // 4. 1を利用してParticleのInstance描画を行う

    engine_->ApplyGpuParticlePSO();

    engine_->SetBlend(BlendMode::kBlendModeAdd);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable());
    engine_->SetCull(PSOManager::CullMode::None);
      
    drawManager_->DrawParticleGPU(
        vertexBufferView_,
        materialResource_->GetGPUVirtualAddress(),
        perViewResource_->GetGPUVirtualAddress(),
        particleSrvHandleGPU_,
        textureHandle_,
        kMaxParticles
    );
}

// デバッグ
void GPUParticleSystem::Debug() {

}