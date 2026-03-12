#include "VoxelParticleSystem.h"
#include "Engine/IrufemiEngine.h"
#include "Resource/Model/ModelManager.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/DirectX/DescriptorPool.h"
#include "Application/camera/Camera.h"
#include "Engine/Core/Math/Geometry/Math.h"
#include <cassert>

IrufemiEngine* VoxelParticleSystem::engine_ = nullptr;

void VoxelParticleSystem::Initialize(
    const std::string& modelName,
    const Vector3Int& resolution,
    Camera* camera)
{
    assert(engine_);
    assert(camera);
    camera_ = camera;
    device_ = engine_->GetDevice();
    modelManager_ = engine_->GetObjModelManager();
    textureManager_ = engine_->GetTextureManager();

    // 1. モデルをボクセル化
    auto managedModel = modelManager_->GetModel(modelName);
    if (!managedModel || !managedModel->cpuModel) {
        assert(false && "Failed to get model for voxelization.");
        return;
    }
    voxelModel_ = std::make_unique<VoxelizedModel>(
        ModelManager::VoxelizeModel(*managedModel->cpuModel, resolution, textureManager_)
    );
    voxelCount_ = static_cast<uint32_t>(voxelModel_->voxels.size());
    if (voxelCount_ == 0) {
        return; // ボクセルがなければ何もしない
    }

    // 2. GPUリソースの作成
    CreateResources();

    // 3. PSOの作成
    CreatePSO();

    // 4. 定数バッファのマッピング
    HRESULT hr = emitterConstantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedEmitterData_));
    assert(SUCCEEDED(hr));
    hr = perViewConstantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedPerViewData_));
    assert(SUCCEEDED(hr));
}

void VoxelParticleSystem::Update(float deltaTime)
{
    if (voxelCount_ == 0) return;

    // エミッターの時間を更新
    // Note: isEmitting_は1フレームのみtrueになるため、ここで時間を加算すると
    //       次のフレームで更新されません。Emit時にリセットされるので問題ないかもしれませんが、
    //       意図しない挙動の可能性もあります。
    emitterData_.time += deltaTime;

    // 定数バッファを更新
    emitterData_.emit = isEmitting_ ? 1 : 0;
    *mappedEmitterData_ = emitterData_;

    // PerView更新
    mappedPerViewData_->viewProjection = camera_->GetViewProjectionMatrix3D();
    
    // backToFrontMatrix_の設定(面の向きをカメラの方向にしてあるのでここは調整なし。0でOK)
    Matrix4x4 backToFrontMatrix_ = Math::MakeRotateYMatrix(0.0f);

    /// カメラの回転を適用する
    Matrix4x4 billboardMatrix_ = Math::Multiply(backToFrontMatrix_, camera_->GetCameraMatrix());
    billboardMatrix_.m[3][0] = 0.0f;
    billboardMatrix_.m[3][1] = 0.0f;
    billboardMatrix_.m[3][2] = 0.0f;

    mappedPerViewData_->billboard = billboardMatrix_;


    ID3D12GraphicsCommandList* commandList = engine_->GetCommandList();

    // --- 初期化 or 更新フェーズ ---
    commandList->SetComputeRootSignature(engine_->GetDirectXCommon()->GetComputeRootSignature());

    if (isEmitting_) {
        // パーティクル初期化
        commandList->SetPipelineState(initializePSO_.Get());
        // RootParameter: [0]SRV, [1]SRV, [2]SRV, [3]UAV, [4]CBV, [5]CBV, [6]UAV, [7]UAV
        commandList->SetComputeRootDescriptorTable(0, voxelSrvHandleGPU_); // t0
        commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_); // u0
        commandList->SetComputeRootConstantBufferView(4, emitterConstantBuffer_->GetGPUVirtualAddress()); // b0
        commandList->Dispatch((voxelCount_ + 1023) / 1024, 1, 1);
        isEmitting_ = false; // 1フレームだけ実行
    } else {
        // パーティクル更新
        commandList->SetPipelineState(updatePSO_.Get());
        commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_); // u0
        commandList->SetComputeRootConstantBufferView(4, emitterConstantBuffer_->GetGPUVirtualAddress()); // b0
        commandList->Dispatch((voxelCount_ + 1023) / 1024, 1, 1);
    }

    // UAVバリア
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = particleBuffer_.Get();
    commandList->ResourceBarrier(1, &barrier);
}

void VoxelParticleSystem::Draw()
{
    if (voxelCount_ == 0) return;

    ID3D12GraphicsCommandList* commandList = engine_->GetCommandList();

    commandList->SetPipelineState(drawPSO_.Get());
    commandList->SetGraphicsRootSignature(engine_->GetDirectXCommon()->GetRootSignature());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    // RootParameter: [0]CBV, [1]CBV, [2]SRV, [3]CBV, [4]SRV, ...
    commandList->SetGraphicsRootConstantBufferView(1, perViewConstantBuffer_->GetGPUVirtualAddress()); // b0 (TransformationMatrix)
    commandList->SetGraphicsRootDescriptorTable(4, particleSrvHandleGPU_); // t0 (Instancing)

    commandList->DrawInstanced(voxelCount_, 1, 0, 0);
}

void VoxelParticleSystem::Emit(const Vector3& position)
{
    emitterData_.emitPosition = position;
    emitterData_.time = 0.0f;
    isEmitting_ = true;
}

void VoxelParticleSystem::CreateResources()
{
    auto* dxCommon = engine_->GetDirectXCommon();
    auto* srvPool = dxCommon->GetSrvPool();

    // Voxelデータ用バッファ (SRV)
    voxelBuffer_ = dxCommon->CreateBufferResource(sizeof(Voxel) * voxelCount_);
    Voxel* voxelData = nullptr;
    voxelBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&voxelData));
    std::memcpy(voxelData, voxelModel_->voxels.data(), sizeof(Voxel) * voxelCount_);
    voxelBuffer_->Unmap(0, nullptr);

    uint32_t voxelSrvIndex = srvPool->Allocate();
    voxelSrvHandleCPU_ = srvPool->GetCPUHandle(voxelSrvIndex);
    voxelSrvHandleGPU_ = srvPool->GetGPUHandle(voxelSrvIndex);

    D3D12_SHADER_RESOURCE_VIEW_DESC voxelSrvDesc{};
    voxelSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    voxelSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    voxelSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    voxelSrvDesc.Buffer.FirstElement = 0;
    voxelSrvDesc.Buffer.NumElements = voxelCount_;
    voxelSrvDesc.Buffer.StructureByteStride = sizeof(Voxel);
    device_->CreateShaderResourceView(voxelBuffer_.Get(), &voxelSrvDesc, voxelSrvHandleCPU_);

    // Particleデータ用バッファ (UAV & SRV)
    particleBuffer_ = dxCommon->CreateUAVBufferResource(sizeof(VoxelParticle) * voxelCount_);
    
    // UAV
    uint32_t particleUavIndex = srvPool->Allocate();
    particleUavHandleCPU_ = srvPool->GetCPUHandle(particleUavIndex);
    particleUavHandleGPU_ = srvPool->GetGPUHandle(particleUavIndex);

    D3D12_UNORDERED_ACCESS_VIEW_DESC particleUavDesc{};
    particleUavDesc.Format = DXGI_FORMAT_UNKNOWN;
    particleUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    particleUavDesc.Buffer.FirstElement = 0;
    particleUavDesc.Buffer.NumElements = voxelCount_;
    particleUavDesc.Buffer.StructureByteStride = sizeof(VoxelParticle);
    device_->CreateUnorderedAccessView(particleBuffer_.Get(), nullptr, &particleUavDesc, particleUavHandleCPU_);

    // SRV
    uint32_t particleSrvIndex = srvPool->Allocate();
    particleSrvHandleCPU_ = srvPool->GetCPUHandle(particleSrvIndex);
    particleSrvHandleGPU_ = srvPool->GetGPUHandle(particleSrvIndex);

    D3D12_SHADER_RESOURCE_VIEW_DESC particleSrvDesc{};
    particleSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    particleSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    particleSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    particleSrvDesc.Buffer.FirstElement = 0;
    particleSrvDesc.Buffer.NumElements = voxelCount_;
    particleSrvDesc.Buffer.StructureByteStride = sizeof(VoxelParticle);
    device_->CreateShaderResourceView(particleBuffer_.Get(), &particleSrvDesc, particleSrvHandleCPU_);


    // Emitter定数バッファ
    emitterConstantBuffer_ = dxCommon->CreateBufferResource(sizeof(VoxelEmitter));
    // PerView定数バッファ
    perViewConstantBuffer_ = dxCommon->CreateBufferResource(sizeof(PerView));
}

void VoxelParticleSystem::CreatePSO()
{
    auto* dxCommon = engine_->GetDirectXCommon();
    auto* psoManager = dxCommon->GetPSOManager();

    // --- Compute PSO ---
    initializePSO_ = dxCommon->GetVoxelParticleInitializePSO();
    updatePSO_ = dxCommon->GetVoxelParticleUpdatePSO();
    assert(initializePSO_ && updatePSO_);

    // --- Graphics PSO ---
    drawPSO_ = psoManager->GetVoxelParticle(BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None);
    assert(drawPSO_);
}
