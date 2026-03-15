#define NOMINMAX
#include "VoxelParticleSystem.h"
#include "Application/camera/Camera.h"
#include "Engine/Core/Math/Geometry/Math.h"
#include "Engine/Graphics/DirectX/DescriptorPool.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Manager/DrawManager.h"
#include "Renderer/VertexData.h"
#include "Resource/Model/ModelManager.h"
#include <cassert>

IrufemiEngine *VoxelParticleSystem::engine_ = nullptr;

void VoxelParticleSystem::Initialize(const std::string &modelName,
                                     const Vector3Int &resolution,
                                     Camera *camera) {
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
  voxelModel_ = std::make_unique<VoxelizedModel>(ModelManager::VoxelizeModel(
      *managedModel->cpuModel, resolution, textureManager_));
  voxelCount_ = static_cast<uint32_t>(voxelModel_->voxels.size());
  if (voxelCount_ == 0) {
    return; // ボクセルがなければ何もしない
  }

  // 2. 立方体メッシュの作成 (ボクセルサイズを計算して渡す)
  float voxelW = (voxelModel_->aabbMax.x - voxelModel_->aabbMin.x) /
                 voxelModel_->resolution.x;
  float voxelH = (voxelModel_->aabbMax.y - voxelModel_->aabbMin.y) /
                 voxelModel_->resolution.y;
  float voxelD = (voxelModel_->aabbMax.z - voxelModel_->aabbMin.z) /
                 voxelModel_->resolution.z;
  CreateCubeMesh(voxelW, voxelH, voxelD);

  // 3. GPUリソースの作成
  CreateResources();

  // 4. PSOの作成
  CreatePSO();

  // 5. 定数バッファのマッピング
  HRESULT hr = emitterConstantBuffer_->Map(
      0, nullptr, reinterpret_cast<void **>(&mappedEmitterData_));
  assert(SUCCEEDED(hr));
  hr = perViewConstantBuffer_->Map(
      0, nullptr, reinterpret_cast<void **>(&mappedPerViewData_));
  assert(SUCCEEDED(hr));

  // 6. GPU上での初期化 (全パーティクルをisActive=0にする)
  ID3D12GraphicsCommandList *commandList = engine_->GetCommandList();
  auto *dxCommon = engine_->GetDirectXCommon();
  ID3D12DescriptorHeap *ppHeaps[] = {dxCommon->GetSrvPool()->GetHeap()};
  commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
  commandList->SetComputeRootSignature(dxCommon->GetComputeRootSignature());
  commandList->SetPipelineState(initializePSO_.Get());
  commandList->SetComputeRootDescriptorTable(0, voxelSrvHandleGPU_);    // t0
  commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_); // u0
  commandList->Dispatch((voxelCount_ + 63) / 64, 1, 1);

  // UAVバリア
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barrier.UAV.pResource = particleBuffer_.Get();
  commandList->ResourceBarrier(1, &barrier);
}

void VoxelParticleSystem::Update(float deltaTime) {
  if (voxelCount_ == 0)
    return;

  // エミッターデータ更新
  emitterData_.time += deltaTime;
  emitterData_.emit = isEmitting_ ? 1 : 0;
  *mappedEmitterData_ = emitterData_;

  // PerFrame データを更新（time と deltaTime を CS シェーダーへ渡す）
  perFrameData_.time = emitterData_.time;
  perFrameData_.deltaTime = deltaTime;
  *mappedPerFrameData_ = perFrameData_;

  // PerView 更新（描画用）
  mappedPerViewData_->viewProjection = camera_->GetViewProjectionMatrix3D();
  mappedPerViewData_->billbordMatrix = Math::MakeIdentity4x4();

  ID3D12GraphicsCommandList *commandList = engine_->GetCommandList();
  auto *dxCommon = engine_->GetDirectXCommon();

  ID3D12DescriptorHeap *ppHeaps[] = {dxCommon->GetSrvPool()->GetHeap()};
  commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
  commandList->SetComputeRootSignature(dxCommon->GetComputeRootSignature());

  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barrier.UAV.pResource = particleBuffer_.Get();

  // 1. エミット処理 (フラクチャリングのトリガー)
  if (isEmitting_) {
    commandList->SetPipelineState(emitPSO_.Get());
    commandList->SetComputeRootDescriptorTable(0, voxelSrvHandleGPU_);    // t0
    commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_); // u0
    commandList->SetComputeRootConstantBufferView(
        4, emitterConstantBuffer_->GetGPUVirtualAddress()); // b0 (VoxelEmitter)
    commandList->SetComputeRootConstantBufferView(
        5, perFrameConstantBuffer_
               ->GetGPUVirtualAddress()); // b1 (PerFrame: time/deltaTime)
    commandList->Dispatch((voxelCount_ + 63) / 64, 1, 1);

    commandList->ResourceBarrier(1, &barrier);

    isEmitting_ = false; // 1度だけ発生
  }

  // 2. 毎フレームの更新処理
  commandList->SetPipelineState(updatePSO_.Get());
  commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_); // u0
  commandList->SetComputeRootConstantBufferView(
      4, emitterConstantBuffer_->GetGPUVirtualAddress()); // b0 (VoxelEmitter)
  commandList->SetComputeRootConstantBufferView(
      5, perFrameConstantBuffer_
             ->GetGPUVirtualAddress()); // b1 (PerFrame: time/deltaTime)
  commandList->Dispatch((voxelCount_ + 63) / 64, 1, 1);

  commandList->ResourceBarrier(1, &barrier);
}

void VoxelParticleSystem::Draw() {
  if (voxelCount_ == 0)
    return;

  ID3D12GraphicsCommandList *commandList = engine_->GetCommandList();
  auto *dxCommon = engine_->GetDirectXCommon();

  // 3DオブジェクトとしてのPSO設定：通常ブレンド・深度書き込み有効・背面カリング
  engine_->SetBlend(BlendMode::kBlendModeNormal);
  engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
  engine_->SetCull(PSOManager::CullMode::Back);

  // VoxelParticle 専用PSOを取得してバインド
  auto *pso = dxCommon->GetPSOManager()->GetVoxelParticle(
      BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable,
      PSOManager::CullMode::Back);
  assert(pso && "VoxelParticle PSO is null.");
  engine_->GetDrawManager()->BindPSO(pso);

  commandList->SetGraphicsRootSignature(dxCommon->GetRootSignature());
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->IASetVertexBuffers(0, 1, &cubeVertexBufferView_);
  commandList->IASetIndexBuffer(&cubeIndexBufferView_);

  // RootParameter: [1]CBV(PerView), [4]SRV(ParticleData)
  commandList->SetGraphicsRootConstantBufferView(
      1, perViewConstantBuffer_->GetGPUVirtualAddress()); // b0 (PerView)
  commandList->SetGraphicsRootDescriptorTable(
      4, particleSrvHandleGPU_); // t0 (ParticleData)

  commandList->DrawIndexedInstanced(cubeIndexCount_, voxelCount_, 0, 0, 0);
}

void VoxelParticleSystem::Emit(const Vector3 &position) {
  emitterData_.emitPosition = position;
  emitterData_.time = 0.0f;
  isEmitting_ = true;
}

void VoxelParticleSystem::CreateCubeMesh(float sizeX, float sizeY,
                                         float sizeZ) {
  auto *dxCommon = engine_->GetDirectXCommon();
  std::vector<VertexData> vertices;
  std::vector<uint32_t> indices;

  // 立方体(中心原点、各軸の辺長 = sizeX/Y/Z)
  const float hx = sizeX * 0.5f;
  const float hy = sizeY * 0.5f;
  const float hz = sizeZ * 0.5f;
  vertices = {
      // 前
      {{-hx, -hy, -hz, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
      {{-hx, hy, -hz, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
      {{hx, hy, -hz, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
      {{hx, -hy, -hz, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
      // 後
      {{hx, -hy, hz, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
      {{hx, hy, hz, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
      {{-hx, hy, hz, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
      {{-hx, -hy, hz, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
      // 左
      {{-hx, -hy, hz, 1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
      {{-hx, hy, hz, 1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
      {{-hx, hy, -hz, 1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
      {{-hx, -hy, -hz, 1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
      // 右
      {{hx, -hy, -hz, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
      {{hx, hy, -hz, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
      {{hx, hy, hz, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
      {{hx, -hy, hz, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
      // 下
      {{-hx, -hy, hz, 1.0f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
      {{-hx, -hy, -hz, 1.0f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
      {{hx, -hy, -hz, 1.0f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
      {{hx, -hy, hz, 1.0f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
      // 上
      {{-hx, hy, -hz, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
      {{-hx, hy, hz, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
      {{hx, hy, hz, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
      {{hx, hy, -hz, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
  };
  indices = {
      0,  1,  2,  0,  2,  3,  // 前
      4,  5,  6,  4,  6,  7,  // 後
      8,  9,  10, 8,  10, 11, // 左
      12, 13, 14, 12, 14, 15, // 右
      16, 17, 18, 16, 18, 19, // 下
      20, 21, 22, 20, 22, 23, // 上
  };
  cubeIndexCount_ = static_cast<uint32_t>(indices.size());

  // Vertex Buffer
  cubeVertexBuffer_ =
      dxCommon->CreateBufferResource(sizeof(VertexData) * vertices.size());
  VertexData *vertexData = nullptr;
  cubeVertexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));
  std::memcpy(vertexData, vertices.data(),
              sizeof(VertexData) * vertices.size());
  cubeVertexBuffer_->Unmap(0, nullptr);

  cubeVertexBufferView_.BufferLocation =
      cubeVertexBuffer_->GetGPUVirtualAddress();
  cubeVertexBufferView_.SizeInBytes =
      sizeof(VertexData) * static_cast<UINT>(vertices.size());
  cubeVertexBufferView_.StrideInBytes = sizeof(VertexData);

  // Index Buffer
  cubeIndexBuffer_ =
      dxCommon->CreateBufferResource(sizeof(uint32_t) * indices.size());
  uint32_t *indexData = nullptr;
  cubeIndexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
  std::memcpy(indexData, indices.data(), sizeof(uint32_t) * indices.size());
  cubeIndexBuffer_->Unmap(0, nullptr);

  cubeIndexBufferView_.BufferLocation =
      cubeIndexBuffer_->GetGPUVirtualAddress();
  cubeIndexBufferView_.SizeInBytes =
      sizeof(uint32_t) * static_cast<UINT>(indices.size());
  cubeIndexBufferView_.Format = DXGI_FORMAT_R32_UINT;
}

void VoxelParticleSystem::CreateResources() {
  auto *dxCommon = engine_->GetDirectXCommon();
  auto *srvPool = dxCommon->GetSrvPool();

  // Voxelデータ用バッファ (SRV)
  voxelBuffer_ = dxCommon->CreateBufferResource(sizeof(Voxel) * voxelCount_);
  Voxel *voxelData = nullptr;
  voxelBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&voxelData));
  std::memcpy(voxelData, voxelModel_->voxels.data(),
              sizeof(Voxel) * voxelCount_);
  voxelBuffer_->Unmap(0, nullptr);

  uint32_t voxelSrvIndex = srvPool->Allocate();
  voxelSrvHandleCPU_ = srvPool->GetCPUHandle(voxelSrvIndex);
  voxelSrvHandleGPU_ = srvPool->GetGPUHandle(voxelSrvIndex);

  D3D12_SHADER_RESOURCE_VIEW_DESC voxelSrvDesc{};
  voxelSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
  voxelSrvDesc.Shader4ComponentMapping =
      D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  voxelSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  voxelSrvDesc.Buffer.FirstElement = 0;
  voxelSrvDesc.Buffer.NumElements = voxelCount_;
  voxelSrvDesc.Buffer.StructureByteStride = sizeof(Voxel);
  device_->CreateShaderResourceView(voxelBuffer_.Get(), &voxelSrvDesc,
                                    voxelSrvHandleCPU_);

  // Particleデータ用バッファ (UAV & SRV)
  particleBuffer_ =
      dxCommon->CreateUAVBufferResource(sizeof(VoxelParticle) * voxelCount_);

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
  device_->CreateUnorderedAccessView(particleBuffer_.Get(), nullptr,
                                     &particleUavDesc, particleUavHandleCPU_);

  // SRV
  uint32_t particleSrvIndex = srvPool->Allocate();
  particleSrvHandleCPU_ = srvPool->GetCPUHandle(particleSrvIndex);
  particleSrvHandleGPU_ = srvPool->GetGPUHandle(particleSrvIndex);

  D3D12_SHADER_RESOURCE_VIEW_DESC particleSrvDesc{};
  particleSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
  particleSrvDesc.Shader4ComponentMapping =
      D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  particleSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  particleSrvDesc.Buffer.FirstElement = 0;
  particleSrvDesc.Buffer.NumElements = voxelCount_;
  particleSrvDesc.Buffer.StructureByteStride = sizeof(VoxelParticle);
  device_->CreateShaderResourceView(particleBuffer_.Get(), &particleSrvDesc,
                                    particleSrvHandleCPU_);

  // Emitter定数バッファ
  emitterConstantBuffer_ = dxCommon->CreateBufferResource(sizeof(VoxelEmitter));
  emitterConstantBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&mappedEmitterData_));
  
  // PerView定数バッファ
  perViewConstantBuffer_ = dxCommon->CreateBufferResource(sizeof(PerView));
  perViewConstantBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&mappedPerViewData_));

  // PerFrame定数バッファ
  perFrameConstantBuffer_ = dxCommon->CreateBufferResource(sizeof(PerFrame));
  perFrameConstantBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&mappedPerFrameData_));
}

void VoxelParticleSystem::CreatePSO() {
  auto *dxCommon = engine_->GetDirectXCommon();
  auto *psoManager = dxCommon->GetPSOManager();

  // --- Compute PSO ---
  initializePSO_ = dxCommon->GetVoxelParticleInitializePSO();
  emitPSO_ = dxCommon->GetVoxelParticleEmitPSO();
  updatePSO_ = dxCommon->GetVoxelParticleUpdatePSO();
  assert(initializePSO_ && emitPSO_ && updatePSO_);

  // --- Graphics PSO ---
  drawPSO_ = psoManager->GetVoxelParticle(BlendMode::kBlendModeAdd,
                                          PSOManager::DepthWrite::Disable,
                                          PSOManager::CullMode::None);
  assert(drawPSO_);
}

void VoxelParticleSystem::Debug(const char *name) {
  if (ImGui::TreeNode(name)) {
    ImGui::Text("Voxel Count: %u", voxelCount_);
    ImGui::DragFloat3("Emit Position", &emitterData_.emitPosition.x, 0.1f);
    if (ImGui::Button("Emit")) {
      Emit(emitterData_.emitPosition);
    }

    ImGui::Separator();
    ImGui::Text("Emitter Settings");
    ImGui::DragFloat("Life Time", &emitterData_.lifeTime, 0.1f, 0.1f, 10.0f);
    ImGui::DragFloat("Gravity", &emitterData_.gravity, 0.1f, -20.0f, 20.0f);
    ImGui::DragFloat("Dispersion", &emitterData_.dispersion, 0.1f, 0.0f, 20.0f);
    ImGui::DragFloat("Convergence", &emitterData_.convergence, 0.1f, 0.0f,
                     20.0f);

    ImGui::TreePop();
  }
}
