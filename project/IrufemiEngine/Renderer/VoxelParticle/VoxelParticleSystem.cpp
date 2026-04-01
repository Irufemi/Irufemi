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
#include <cstdio>
#include "Engine/Core/Math/Geometry/OBB.h"
#include <Windows.h>
#include <algorithm>

IrufemiEngine *VoxelParticleSystem::engine_ = nullptr;

VoxelParticleSystem::~VoxelParticleSystem() {
  if (initializeFuture_.valid()) {
    initializeFuture_.wait();
  }
  if (engine_) {
    if (auto *srvPool = engine_->GetSrvPool()) {
      uint64_t fv = engine_->GetDirectXCommon()->GetFenceValue();
      srvPool->FreeAfterFence(voxelSrvIndex_, fv);
      srvPool->FreeAfterFence(particleUavIndex_, fv);
      srvPool->FreeAfterFence(particleSrvIndex_, fv);
    }
  }
}

void VoxelParticleSystem::Initialize(const std::string &modelName,
                                     const Vector3Int &resolution,
                                     Camera *camera) {
  assert(engine_);
  assert(camera);
  camera_ = camera;
  device_ = engine_->GetDevice();
  modelManager_ = engine_->GetObjModelManager();
  textureManager_ = engine_->GetTextureManager();

  status_.store(LoadingStatus::Loading);

  // 非同期でボクセル化を開始
  initializeFuture_ = modelManager_->EnqueueTask([this, modelName, resolution]() {
    auto managedModel = modelManager_->GetModel(modelName);
    if (!managedModel || !managedModel->cpuModel) {
      status_.store(LoadingStatus::Failed);
      assert(false && "Failed to get model for voxelization.");
      return;
    }

    // 重い計算（ボクセル化）
    auto vModel = std::make_unique<VoxelizedModel>(ModelManager::VoxelizeModel(
        *managedModel->cpuModel, resolution, textureManager_));

    {
      std::lock_guard<std::mutex> lock(voxelModelMutex_);
      voxelModel_ = std::move(vModel);
      voxelCount_ = static_cast<uint32_t>(voxelModel_->voxels.size());
    }

    if (voxelCount_ == 0) {
      status_.store(LoadingStatus::Failed);
      OutputDebugStringA("[Voxel] ERROR: Voxel count is ZERO.\n");
      return;
    }

    // 計算完了
    status_.store(LoadingStatus::ReadyToCreateResources);
  });
}

void VoxelParticleSystem::FinishInitialization() {
  if (status_.load() != LoadingStatus::ReadyToCreateResources) {
    return;
  }

  // 1. 立方体メッシュの作成 (ボクセルサイズを計算して渡す)
  float voxelW, voxelH, voxelD;
  {
    std::lock_guard<std::mutex> lock(voxelModelMutex_);
    voxelW = (voxelModel_->aabbMax.x - voxelModel_->aabbMin.x) /
                   voxelModel_->resolution.x;
    voxelH = (voxelModel_->aabbMax.y - voxelModel_->aabbMin.y) /
                   voxelModel_->resolution.y;
    voxelD = (voxelModel_->aabbMax.z - voxelModel_->aabbMin.z) /
                   voxelModel_->resolution.z;
  }
  CreateCubeMesh(voxelW, voxelH, voxelD);

  // 2. GPUリソースの作成
  CreateResources();

  // 3. PSOの作成
  CreatePSO();

  // 4. 定数バッファのマッピング
  HRESULT hr = emitterConstantBuffer_->Map(
      0, nullptr, reinterpret_cast<void **>(&mappedEmitterData_));
  assert(SUCCEEDED(hr));
  hr = perViewConstantBuffer_->Map(
      0, nullptr, reinterpret_cast<void **>(&mappedPerViewData_));
  assert(SUCCEEDED(hr));
  hr = perFrameConstantBuffer_->Map(
      0, nullptr, reinterpret_cast<void **>(&mappedPerFrameData_));
  assert(SUCCEEDED(hr));

  needsInitialize_ = true;
  status_.store(LoadingStatus::Loaded);

  char log[256];
  sprintf_s(log, "[Voxel] Async Initialization Finished. Count: %u\n", voxelCount_);
  OutputDebugStringA(log);
}

void VoxelParticleSystem::Update(float deltaTime) {
  if (status_.load() == LoadingStatus::ReadyToCreateResources) {
    FinishInitialization();
  }

  if (status_.load() != LoadingStatus::Loaded || voxelCount_ == 0)
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
  
  // ビルボード行列の計算
  Matrix4x4 backToFrontMatrix = Math::MakeRotateYMatrix(0.0f);
  mappedPerViewData_->billboardMatrix = Math::Multiply(backToFrontMatrix, camera_->GetCameraMatrix());
  mappedPerViewData_->billboardMatrix.m[3][0] = 0.0f;
  mappedPerViewData_->billboardMatrix.m[3][1] = 0.0f;
  mappedPerViewData_->billboardMatrix.m[3][2] = 0.0f;

  // Dispatch 処理は Draw に移動 (PreDraw 後に実行するため)
}

void VoxelParticleSystem::Draw() {
  if (status_.load() != LoadingStatus::Loaded || !voxelBuffer_ || !engine_ || !camera_)
    return;

  ID3D12GraphicsCommandList *commandList = engine_->GetCommandList();
  auto *dxCommon = engine_->GetDirectXCommon();

  // 1. Compute Shader dispatch (Deferred from Initialize and Update)

  // デスクリプタヒープの設定
  ID3D12DescriptorHeap *ppHeaps[] = {dxCommon->GetSrvPool()->GetHeap()};
  commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
  commandList->SetComputeRootSignature(dxCommon->GetComputeRootSignature());

  D3D12_RESOURCE_BARRIER uavBarrier{};
  uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  uavBarrier.UAV.pResource = particleBuffer_.Get();

  // A. 初期化
  if (needsInitialize_) {
    commandList->SetPipelineState(initializePSO_.Get());
    commandList->SetComputeRootDescriptorTable(0, voxelSrvHandleGPU_);    // t0
    commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_); // u0
    commandList->Dispatch((voxelCount_ + 63) / 64, 1, 1);
    commandList->ResourceBarrier(1, &uavBarrier);
    needsInitialize_ = false;
  }

  // B. エミット
  if (isEmitting_) {
    commandList->SetPipelineState(emitPSO_.Get());
    commandList->SetComputeRootDescriptorTable(0, voxelSrvHandleGPU_);    // t0
    commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_); // u0
    commandList->SetComputeRootConstantBufferView(4, emitterConstantBuffer_->GetGPUVirtualAddress());
    commandList->SetComputeRootConstantBufferView(5, perFrameConstantBuffer_->GetGPUVirtualAddress());
    commandList->Dispatch((voxelCount_ + 63) / 64, 1, 1);
    commandList->ResourceBarrier(1, &uavBarrier);
    isEmitting_ = false;
  }

  // C. 毎フレームの更新
  commandList->SetPipelineState(updatePSO_.Get());
  commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_); // u0
  commandList->SetComputeRootConstantBufferView(4, emitterConstantBuffer_->GetGPUVirtualAddress());
  commandList->SetComputeRootConstantBufferView(5, perFrameConstantBuffer_->GetGPUVirtualAddress());
  commandList->Dispatch((voxelCount_ + 63) / 64, 1, 1);
  commandList->ResourceBarrier(1, &uavBarrier);

  // --- デバッグログ ---
  if (++debugFrameCount_ >= 60) {
    debugFrameCount_ = 0;
    char logMsg[128];
    sprintf_s(logMsg, "[Voxel Draw][Ptr:%p] count:%u hasExploded:%d\n",
              this, voxelCount_, hasExploded_ ? 1 : 0);
    OutputDebugStringA(logMsg);
  }

  // 2. Graphics Draw
  if (!hasExploded_)
    return;

  // リソースバリヤー: UAV -> ShaderResource (読み取り)
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = particleBuffer_.Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  commandList->ResourceBarrier(1, &barrier);

  // VoxelParticle 専用PSOを取得してバインド
  engine_->GetDrawManager()->BindPSO(drawPSO_.Get());

  // コンピュートシェーダー実行後にグラフィックスのルートシグネチャと共通パラメータを再バインド
  engine_->GetDrawManager()->BindCommonParameters();
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->IASetVertexBuffers(0, 1, &cubeVertexBufferView_);
  commandList->IASetIndexBuffer(&cubeIndexBufferView_);

  // RootParameter: [8]CBV(PerView), [1]CBV(VoxelEmitter), [11]SRV(ParticleData)
  commandList->SetGraphicsRootConstantBufferView(
      8, perViewConstantBuffer_->GetGPUVirtualAddress()); // b6 (PerView)
  commandList->SetGraphicsRootConstantBufferView(
      1, emitterConstantBuffer_->GetGPUVirtualAddress()); // b0 (VoxelEmitter)
  commandList->SetGraphicsRootDescriptorTable(
      11, particleSrvHandleGPU_); // t1 (ParticleData)

  commandList->DrawIndexedInstanced(cubeIndexCount_, voxelCount_, 0, 0, 0);

  // リソースバリヤー: ShaderResource -> UAV (次のフレームの計算用に戻す)
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  commandList->ResourceBarrier(1, &barrier);
}

void VoxelParticleSystem::Emit(const Vector3 &position) {
  emitterData_.emitPosition = position;
  emitterData_.baseVelocity = {0, 0, 0};
  emitterData_.rotate = {0, 0, 0};
  emitterData_.scale = {1, 1, 1};
  emitterData_.convergence = 0.0f;
  emitterData_.gravity = 9.8f;
  emitterData_.dispersion = 5.0f;
  emitterData_.lifeTime = 2.0f;
  emitterData_.time = 0.0f;
  emitterData_.useCollision = 0; // 衝突判定無効
  isEmitting_ = true;
  hasExploded_ = true;
}

void VoxelParticleSystem::Explode(const Vector3 &position,
                                  const Vector3 &velocity,
                                  const Vector3 &rotate,
                                  const Vector3 &scale) {
  emitterData_.emitPosition = position;
  emitterData_.baseVelocity = velocity;
  emitterData_.rotate = rotate;
  emitterData_.scale = scale;
  emitterData_.time = 0.0f;
  emitterData_.useCollision = 0; // 衝突判定無効
  isEmitting_ = true;
  hasExploded_ = true;
}

void VoxelParticleSystem::CollisionScatter(const Vector3 &position,
                                           const Vector3 &velocity,
                                           const Vector3 &rotate,
                                           const Vector3 &scale,
                                           const OBB &collisionArea) {
  if (isEmitting_) {
    // 既にエミット待ちの場合は、領域を広げて両方の衝突をカバーするようにする
    // 簡易的に AABB ベースで合成領域を計算
    Vector3 minA = Math::Subtract(emitterData_.collisionCenter, emitterData_.collisionSize);
    Vector3 maxA = Math::Add(emitterData_.collisionCenter, emitterData_.collisionSize);
    Vector3 minB = Math::Subtract(collisionArea.center, collisionArea.size);
    Vector3 maxB = Math::Add(collisionArea.center, collisionArea.size);

    Vector3 newMin = { (std::min)(minA.x, minB.x), (std::min)(minA.y, minB.y), (std::min)(minA.z, minB.z) };
    Vector3 newMax = { (std::max)(maxA.x, maxB.x), (std::max)(maxA.y, maxB.y), (std::max)(maxA.z, maxB.z) };

    emitterData_.collisionCenter = Math::Multiply(0.5f, Math::Add(newMin, newMax));
    emitterData_.collisionSize = Math::Multiply(0.5f, Math::Subtract(newMax, newMin));
    // 合成後は軸並行（回転なし）として扱う
    for (int i = 0; i < 3; ++i) {
      emitterData_.collisionOrientations[i] = { 0.0f, 0.0f, 0.0f, 0.0f };
    }
    emitterData_.collisionOrientations[0].x = 1.0f;
    emitterData_.collisionOrientations[1].y = 1.0f;
    emitterData_.collisionOrientations[2].z = 1.0f;
  } else {
    emitterData_.emitPosition = position;
    emitterData_.baseVelocity = velocity;
    emitterData_.rotate = rotate;
    emitterData_.scale = scale;
    emitterData_.time = 0.0f;

    // 衝突判定用データ設定
    emitterData_.useCollision = 1;
    emitterData_.collisionCenter = collisionArea.center;
    emitterData_.collisionSize = collisionArea.size;
    for (int i = 0; i < 3; ++i) {
      emitterData_.collisionOrientations[i].x = collisionArea.orientations[i].x;
      emitterData_.collisionOrientations[i].y = collisionArea.orientations[i].y;
      emitterData_.collisionOrientations[i].z = collisionArea.orientations[i].z;
      emitterData_.collisionOrientations[i].w = 0.0f;
    }
  }

  isEmitting_ = true;
  hasExploded_ = true;
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
  voxelSrvIndex_ = voxelSrvIndex;
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
  particleUavIndex_ = particleUavIndex;
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
  particleSrvIndex_ = particleSrvIndex;
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

  // Emitter定数バッファ (256バイトアライメント)
  emitterConstantBuffer_ = dxCommon->CreateBufferResource((sizeof(VoxelEmitter) + 0xFF) & ~0xFF);
  emitterConstantBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&mappedEmitterData_));
  
  // PerView定数バッファ (256バイトアライメント)
  perViewConstantBuffer_ = dxCommon->CreateBufferResource((sizeof(PerView) + 0xFF) & ~0xFF);
  perViewConstantBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&mappedPerViewData_));

  // PerFrame定数バッファ (256バイトアライメント)
  perFrameConstantBuffer_ = dxCommon->CreateBufferResource((sizeof(PerFrame) + 0xFF) & ~0xFF);
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
  drawPSO_ = psoManager->GetVoxelParticle(BlendMode::kBlendModeNormal,
                                          PSOManager::DepthWrite::Enable,
                                          PSOManager::CullMode::Back);
  assert(drawPSO_);
}

void VoxelParticleSystem::Debug([[maybe_unused]] const char *name) {

#ifdef USE_IMGUI

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

#endif // USE_IMGUI

}
