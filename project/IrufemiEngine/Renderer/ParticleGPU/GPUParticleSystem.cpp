#include "GPUParticleSystem.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/DirectX/DescriptorPool.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Core/Math/Math.h"
#include "Resource/Texture/TextureManager.h"
#include "Renderer/VertexData.h"
#include "Application/camera/Camera.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Math/Geometry/Frustum.h"
#include <cassert>

// 静的メンバ変数の実体定義
DirectXCommon* GPUParticleSystem::dxCommon_ = nullptr;
DrawManager* GPUParticleSystem::drawManager_ = nullptr;
TextureManager* GPUParticleSystem::textureManager_ = nullptr;
IrufemiEngine* GPUParticleSystem::engine_ = nullptr;

// コンストラクタ
GPUParticleSystem::GPUParticleSystem() = default;

// デストラクタ
GPUParticleSystem::~GPUParticleSystem() {
    if (auto* srvPool = dxCommon_ ? dxCommon_->GetSrvPool() : nullptr) {
        uint64_t fv = dxCommon_->GetCurrentFrameFenceValue();
        srvPool->FreeAfterFence(emitterSrvIndex_, fv);
        srvPool->FreeAfterFence(perFrameSrvIndex_, fv);
        srvPool->FreeAfterFence(particleUavIndex_, fv);
        srvPool->FreeAfterFence(particleSrvIndex_, fv);
        srvPool->FreeAfterFence(freeListIndexUavIndex_, fv);
        srvPool->FreeAfterFence(freeListUavIndex_, fv);
    }
}

// 初期化
void GPUParticleSystem::Initialize(Camera* camera, const std::string& textureName) {

    assert(dxCommon_);
    assert(drawManager_);
    assert(textureManager_);
    assert(engine_);

    camera_ = camera;

    auto* srvPool = dxCommon_->GetSrvPool();

    /*Emitter*/
    emitterResource_ = dxCommon_->CreateBufferResource(sizeof(GPUParticleEmitter));
    emitterResource_->Map(0, nullptr, reinterpret_cast<void**>(&emitter_));
    *emitter_ = GPUParticleEmitter(); // 初期化

    perFrameResource_ = dxCommon_->CreateBufferResource(sizeof(PerFrame));
    perFrameResource_->Map(0, nullptr, reinterpret_cast<void**>(&perFrameData_));

    // SRV
    emitterSrvIndex_ = srvPool->Allocate();
    emitterSrvHandleCPU_ = srvPool->GetCPUHandle(emitterSrvIndex_);
    emitterSrvHandleGPU_ = srvPool->GetGPUHandle(emitterSrvIndex_);
    D3D12_SHADER_RESOURCE_VIEW_DESC emitterSrvDesc{};
    emitterSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    emitterSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    emitterSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    emitterSrvDesc.Buffer.FirstElement = 0;
    emitterSrvDesc.Buffer.NumElements = 1;
    emitterSrvDesc.Buffer.StructureByteStride = sizeof(GPUParticleEmitter);
    dxCommon_->GetDevice()->CreateShaderResourceView(emitterResource_.Get(), &emitterSrvDesc, emitterSrvHandleCPU_);

    // perFrame SRV
    perFrameSrvIndex_ = srvPool->Allocate();
    perFrameSrvHandleCPU_ = srvPool->GetCPUHandle(perFrameSrvIndex_);
    perFrameSrvHandleGPU_ = srvPool->GetGPUHandle(perFrameSrvIndex_);
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
    particleUavIndex_ = srvPool->Allocate();
    particleUavHandleCPU_ = srvPool->GetCPUHandle(particleUavIndex_);
    particleUavHandleGPU_ = srvPool->GetGPUHandle(particleUavIndex_);
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = kMaxParticles;
    uavDesc.Buffer.StructureByteStride = sizeof(ParticleCS);
    dxCommon_->GetDevice()->CreateUnorderedAccessView(particleResource_.Get(), nullptr, &uavDesc, particleUavHandleCPU_);

    // SRV
    particleSrvIndex_ = srvPool->Allocate();
    particleSrvHandleCPU_ = srvPool->GetCPUHandle(particleSrvIndex_);
    particleSrvHandleGPU_ = srvPool->GetGPUHandle(particleSrvIndex_);
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
    freeListIndexUavIndex_ = srvPool->Allocate();
    freeListIndexUavHandleCPU_ = srvPool->GetCPUHandle(freeListIndexUavIndex_);
    freeListIndexUavHandleGPU_ = srvPool->GetGPUHandle(freeListIndexUavIndex_);
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
    freeListUavIndex_ = srvPool->Allocate();
    freeListUavHandleCPU_ = srvPool->GetCPUHandle(freeListUavIndex_);
    freeListUavHandleGPU_ = srvPool->GetGPUHandle(freeListUavIndex_);
    D3D12_UNORDERED_ACCESS_VIEW_DESC freeListUavDesc{};
    freeListUavDesc.Format = DXGI_FORMAT_UNKNOWN;
    freeListUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    freeListUavDesc.Buffer.FirstElement = 0;
    freeListUavDesc.Buffer.NumElements = kMaxParticles;
    freeListUavDesc.Buffer.StructureByteStride = sizeof(int32_t);
    dxCommon_->GetDevice()->CreateUnorderedAccessView(freeListResource_.Get(), nullptr, &freeListUavDesc, freeListUavHandleCPU_);

    // PerView用リソース
    perViewResource_ = dxCommon_->CreateBufferResource(sizeof(PerView));
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

    // CS初期化フラグを下ろす（実際の初期化はDraw()内で行う）
    isInitializedCS_ = false;

    // デフォルトでスフィアエミッターを設定
    SetSphereEmitter(Vector3(0, 0, 0), 1.0f, 10, 0.5f);
    SetEmit(true);

    perFrameData_->deltaTime = engine_->GetDeltaTime();

}

// 更新
void GPUParticleSystem::Update() {
    if (!emitter_ || !camera_) return;

    isCulled_ = false;
    if (isCullingEnabled_) {
        Sphere boundingSphere;
        boundingSphere.center = emitter_->translate;
        // Boundingを計算。Sphereなら半径*3、Beamなら広めに設定
        if (emitter_->type == 0) {
            boundingSphere.radius = emitter_->radius * 3.0f;
        } else {
            boundingSphere.radius = 50.0f; // ビームは長いので広めに
        }

        if (!Collision::IsCollision(camera_->GetFrustum(), boundingSphere)) {
            isCulled_ = true;
            return; // 画面外なら計算（CS）をスキップ
        }
    }

    /*Particleを発生させる*/

    emitter_->frequencyTime += engine_->GetDeltaTime(); // δタイムを加算
    perFrameData_->time = engine_->GetTotalTime();
    perFrameData_->deltaTime = engine_->GetDeltaTime();

    // 射出間隔を上回ったら射出許可を出して時間を調整
    if (emitter_->frequency <= emitter_->frequencyTime) {
        if (emitter_->emit) { // そもそもEmitフラグが立っている時のみ
            emitter_->frequencyTime -= emitter_->frequency;
            // シェーダー側でこれを見て放出。1フレームに複数回出る可能性は今のところ考慮しない
        } else {
            emitter_->frequencyTime = 0; // Emit停止中ならタイマーリセット
        }
    }

    perViewData_->viewProjection = camera_->GetViewProjectionMatrix3D();

    // backToFrontMatrix_の設定(面の向きをカメラの方向にしてあるのでここは調整なし。0でOK)
    Matrix4x4 backToFrontMatrix_ = Math::MakeRotateYMatrix(0.0f);

    /// カメラの回転を適用する
    Matrix4x4 billboardMatrix_ = Math::Multiply(backToFrontMatrix_, camera_->GetCameraMatrix());
    billboardMatrix_.m[3][0] = 0.0f;
    billboardMatrix_.m[3][1] = 0.0f;
    billboardMatrix_.m[3][2] = 0.0f;

    perViewData_->billboardMatrix = billboardMatrix_;

    needsUpdateCS_ = true;
}

// 描画
void GPUParticleSystem::Draw() {

    if (isCulled_) return;

    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // 0. 未初期化の場合、CSでバッファを初期化する
    if (!isInitializedCS_) {
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
        barrier.UAV.pResource = nullptr;
        commandList->ResourceBarrier(1, &barrier);

        isInitializedCS_ = true;
    }

    // 1. Compute Shader dispatch (Update/Emit) - Only if Update() was called
    if (needsUpdateCS_) {
        ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
        commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

        commandList->SetComputeRootSignature(dxCommon_->GetComputeRootSignature());

        D3D12_RESOURCE_BARRIER uavBarrier{};
        uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarrier.UAV.pResource = nullptr;

        // Emit
        commandList->SetPipelineState(dxCommon_->GetGpuParticleEmitPSO());
        commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_);
        commandList->SetComputeRootDescriptorTable(6, freeListIndexUavHandleGPU_);
        commandList->SetComputeRootDescriptorTable(7, freeListUavHandleGPU_);
        commandList->SetComputeRootConstantBufferView(4, emitterResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootConstantBufferView(5, perFrameResource_->GetGPUVirtualAddress());
        commandList->Dispatch(1, 1, 1);

        commandList->ResourceBarrier(1, &uavBarrier);

        // Update
        commandList->SetPipelineState(dxCommon_->GetGpuParticleUpdatePSO());
        commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_);
        commandList->SetComputeRootDescriptorTable(6, freeListIndexUavHandleGPU_);
        commandList->SetComputeRootDescriptorTable(7, freeListUavHandleGPU_);
        commandList->SetComputeRootConstantBufferView(5, perFrameResource_->GetGPUVirtualAddress());
        commandList->Dispatch((kMaxParticles + 1023) / 1024, 1, 1);

        commandList->ResourceBarrier(1, &uavBarrier);

        needsUpdateCS_ = false;
    }

    // 2. Graphics Draw
    D3D12_RESOURCE_BARRIER transitionBarrier{};
    transitionBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    transitionBarrier.Transition.pResource = particleResource_.Get();
    transitionBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    transitionBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    transitionBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &transitionBarrier);

    engine_->ApplyGpuParticlePSO();
    engine_->SetBlend(BlendMode::kBlendModeAdd);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->SetCull(PSOManager::CullMode::None);
      
    drawManager_->DrawGPUParticle(
        vertexBufferView_,
        materialResource_->GetGPUVirtualAddress(),
        perViewResource_->GetGPUVirtualAddress(),
        particleSrvHandleGPU_,
        textureHandle_,
        kMaxParticles
    );

    transitionBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    transitionBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    commandList->ResourceBarrier(1, &transitionBarrier);
}

// デバッグ
void GPUParticleSystem::Debug() {
#if defined(USE_IMGUI)
    ImGui::Begin("GPUParticleSystem");
    ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);
    if (emitter_) {
        ImGui::Text("Type: %s", emitter_->type == 0 ? "Sphere" : "Beam");
        ImGui::DragFloat3("Translate", &emitter_->translate.x, 0.1f);
        if (emitter_->type == 0) {
            ImGui::DragFloat("Radius", &emitter_->radius, 0.1f, 0.0f, 100.0f);
        } else {
            ImGui::DragFloat3("Direction", &emitter_->direction.x, 0.01f);
            ImGui::DragFloat("Velocity", &emitter_->velocity, 0.1f);
            ImGui::DragFloat("Spread", &emitter_->spread, 0.01f, 0.0f, 1.0f);
        }
        ImGui::DragInt("Count", (int*)&emitter_->count, 1, 0, 100);
        ImGui::DragFloat("Frequency", &emitter_->frequency, 0.01f, 0.001f, 10.0f);
        bool emit = emitter_->emit != 0;
        if (ImGui::Checkbox("Emit", &emit)) {
            emitter_->emit = emit ? 1 : 0;
        }
    }
    ImGui::End();
#endif
}

void GPUParticleSystem::SetSphereEmitter(const Vector3& pos, float radius, uint32_t count, float frequency) {
    if (!emitter_) return;
    emitter_->type = 0;
    emitter_->translate = pos;
    emitter_->radius = radius;
    emitter_->count = (int32_t)count;
    emitter_->frequency = frequency;
}

void GPUParticleSystem::SetBeamEmitter(const Vector3& pos, const Vector3& direction, float radius, float velocity, float spread, uint32_t count, float frequency) {
    if (!emitter_) return;
    emitter_->type = 1;
    emitter_->translate = pos;
    emitter_->direction = direction;
    emitter_->radius = radius;
    emitter_->velocity = velocity;
    emitter_->spread = spread;
    emitter_->count = (int32_t)count;
    emitter_->frequency = frequency;
}