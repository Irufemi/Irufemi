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
#include "Engine/Manager/PrimitiveManager.h"
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
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        emitterResource_[i] = dxCommon_->CreateBufferResource(sizeof(GPUParticleEmitter));
        emitterResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&emitterMapped_[i]));

        perFrameResource_[i] = dxCommon_->CreateBufferResource(sizeof(PerFrame));
        perFrameResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&perFrameMapped_[i]));
    }
    *emitter_ = GPUParticleEmitter(); // 初期化マスター

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
    dxCommon_->GetDevice()->CreateShaderResourceView(emitterResource_[0].Get(), &emitterSrvDesc, emitterSrvHandleCPU_);

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
    dxCommon_->GetDevice()->CreateShaderResourceView(perFrameResource_[0].Get(), &perFrameSrvDesc, perFrameSrvHandleCPU_);


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

    // 形状の初期設定 (デフォルトは Quad/Plane)
    SetPrimitive(PrimitiveType::Plane);

    textureHandle_ = textureManager_->GetTextureHandle(textureName);

    // CS初期化フラグを下ろす（実際の初期化はDraw()内で行う）
    isInitializedCS_ = false;

    // デフォルトでスフィアエミッターを設定
    SetSphereEmitter(Vector3(0, 0, 0), 2.0f, 30, 0.1f);
    
    // Milestone 1: 初期調整 (レガシー演出の復元)
    emitter_->minLife = 0.4f;
    emitter_->maxLife = 0.8f;
    emitter_->startScaleMin = { 0.2f, 0.2f, 0.2f };
    emitter_->startScaleMax = { 0.5f, 0.5f, 0.5f };
    emitter_->endScaleMin = { 0.01f, 0.01f, 0.01f };
    emitter_->endScaleMax = { 0.1f, 0.1f, 0.1f };
    emitter_->startColorMin = { 1.0f, 1.0f, 0.3f, 1.0f }; // 黄色
    emitter_->startColorMax = { 1.0f, 1.0f, 0.4f, 1.0f };
    emitter_->endColorMin = { 1.0f, 0.1f, 0.0f, 0.0f };   // 赤（フェードアウト）
    emitter_->endColorMax = { 1.0f, 0.5f, 0.1f, 0.0f };   // オレンジ（フェードアウト）
    emitter_->colorMode = 0;
    emitter_->gravity = 0.0f;
    emitter_->damping = 0.0f;
    emitter_->jitter = 0.01f; // 座標のゆらぎ
    emitter_->isBillboard = 1;
    emitter_->burstCount = 0;
    
    // Milestone 3: 初期設定
    emitter_->atlasRows = 1;
    emitter_->atlasCols = 1;
    emitter_->groundHeight = -100.0f;
    emitter_->bounce = 0.5f;
    emitter_->attractorStrength = 0.0f;
    emitter_->attractorPos = { 0, 0, 0 };

    SetEmit(true);

    perFrameData_->deltaTime = engine_->GetDeltaTime();

}

// 更新
void GPUParticleSystem::Update() {
    if (!emitter_ || !camera_) return;

    // 前フレームの射出予約をリセット
    emitter_->burstCount = 0;

    if (!isPlaying_) {
        // 放出は止めるが、既存の粒子の更新（Update CS）は必要かもしれないので
        // emit フラグだけ操作して UpdateCS は継続する方針
        emitter_->emit = 0;
    }

    float dt = engine_->GetDeltaTime();

    // 持続時間制御
    if (isPlaying_ && duration_ > 0.0f) {
        totalTime_ += dt;
        if (totalTime_ >= duration_) {
            if (isLooping_) {
                totalTime_ = 0.0f;
                // ※ループ時は必要なら一瞬だけ全クリアする等の処理を検討
            } else {
                Stop();
            }
        }
    }

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

    if (isPlaying_) {
        emitter_->frequencyTime += dt; // δタイムを加算
    }

    perFrameData_->time = engine_->GetTotalTime();
    perFrameData_->deltaTime = dt;

    // 射出間隔を上回ったら射出予約に加算
    if (emitter_->emit) {
        while (emitter_->frequency <= emitter_->frequencyTime && emitter_->frequency > 0.0f) {
            emitter_->burstCount += (uint32_t)emitter_->count;
            emitter_->frequencyTime -= emitter_->frequency;
        }
    } else {
        emitter_->frequencyTime = 0.0f; // Emit停止中ならタイマーリセット
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

    // フレームインデックスを取得して現在のフレーム用のGPUバッファにマスターデータをコピー
    uint32_t frameIndex = dxCommon_->GetFrameIndex();
    *emitterMapped_[frameIndex] = *emitter_;
    *perFrameMapped_[frameIndex] = *perFrameData_;

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

        uint32_t frameIndex = dxCommon_->GetFrameIndex();

        // Emit
        commandList->SetPipelineState(dxCommon_->GetGpuParticleEmitPSO());
        commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_);
        commandList->SetComputeRootDescriptorTable(6, freeListIndexUavHandleGPU_);
        commandList->SetComputeRootDescriptorTable(7, freeListUavHandleGPU_);
        commandList->SetComputeRootConstantBufferView(4, emitterResource_[frameIndex]->GetGPUVirtualAddress());
        commandList->SetComputeRootConstantBufferView(5, perFrameResource_[frameIndex]->GetGPUVirtualAddress());
        commandList->Dispatch(1, 1, 1);

        commandList->ResourceBarrier(1, &uavBarrier);

        // Update
        commandList->SetPipelineState(dxCommon_->GetGpuParticleUpdatePSO());
        commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_);
        commandList->SetComputeRootDescriptorTable(6, freeListIndexUavHandleGPU_);
        commandList->SetComputeRootDescriptorTable(7, freeListUavHandleGPU_);
        commandList->SetComputeRootConstantBufferView(4, emitterResource_[frameIndex]->GetGPUVirtualAddress());
        commandList->SetComputeRootConstantBufferView(5, perFrameResource_[frameIndex]->GetGPUVirtualAddress());
        commandList->Dispatch((kMaxParticles + 1023) / 1024, 1, 1);

        commandList->ResourceBarrier(1, &uavBarrier);

        // burstCountをリセット（EmitParticle.CS で処理した後にリセットしたいが
        // CPU側ですぐリセットすると CS 実行前に 0 になる恐れがある。
        // ただし、Dispatch 直前なのでここでは問題ないはず。本来は CS 内で 0 に落とすのが安全だが
        // StructuredBuffer ではないので不可。
        // 射出予約カウントのリセットは Update 冒頭へ移動

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

    commandList->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    engine_->ApplyGpuParticlePSO();
    engine_->SetBlend(BlendMode::kBlendModeAdd);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->SetCull(PSOManager::CullMode::None);
      
    uint32_t frameIndex = dxCommon_->GetFrameIndex();

    drawManager_->DrawGPUParticle(
        vertexBufferView_,
        indexBufferView_,
        indexCount_,
        materialResource_->GetGPUVirtualAddress(),
        perViewResource_->GetGPUVirtualAddress(),
        emitterResource_[frameIndex]->GetGPUVirtualAddress(),
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
    
    if (ImGui::Button("Play")) Play(); ImGui::SameLine();
    if (ImGui::Button("Stop")) Stop(); ImGui::SameLine();
    if (ImGui::Button("Clear")) Clear();

    ImGui::Separator();

    if (emitter_) {
        const char* typeNames[] = { "Sphere", "Beam", "Ring", "Cylinder" };
        int type = (int)emitter_->type;
        if (ImGui::Combo("Type", &type, typeNames, IM_ARRAYSIZE(typeNames))) {
            emitter_->type = (uint32_t)type;
        }

        ImGui::DragFloat3("Translate", &emitter_->translate.x, 0.1f);
        
        if (emitter_->type == 0 || emitter_->type == 2 || emitter_->type == 3) {
            ImGui::DragFloat("Radius", &emitter_->radius, 0.1f, 0.0f, 100.0f);
        }
        if (emitter_->type == 1) {
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

        ImGui::Separator();
        ImGui::Text("Particle Randomization");
        ImGui::DragFloat2("Life Range", &emitter_->minLife, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat3("Start Scale Min", &emitter_->startScaleMin.x, 0.01f);
        ImGui::DragFloat3("Start Scale Max", &emitter_->startScaleMax.x, 0.01f);
        ImGui::DragFloat3("End Scale Min", &emitter_->endScaleMin.x, 0.01f);
        ImGui::DragFloat3("End Scale Max", &emitter_->endScaleMax.x, 0.01f);

        ImGui::ColorEdit4("Start Color Min", &emitter_->startColorMin.x);
        ImGui::ColorEdit4("Start Color Max", &emitter_->startColorMax.x);
        ImGui::ColorEdit4("End Color Min", &emitter_->endColorMin.x);
        ImGui::ColorEdit4("End Color Max", &emitter_->endColorMax.x);

        ImGui::Separator();
        ImGui::Text("Physics");
        ImGui::DragFloat("Gravity", &emitter_->gravity, 0.01f);
        ImGui::DragFloat("Damping", &emitter_->damping, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Jitter", &emitter_->jitter, 0.001f, 0.0f, 0.5f);
        
        bool billboard = emitter_->isBillboard != 0;
        if (ImGui::Checkbox("Billboard", &billboard)) {
            SetBillboard(billboard);
        }

        ImGui::Separator();
        ImGui::Text("Mesh Shape");
        const char* primitiveNames[] = { "Triangle", "Plane", "Cube", "Cylinder", "Sphere", "Tetra", "Circle", "Ring" };
        int currentPrim = (int)primitiveType_;
        if (ImGui::Combo("Particle Mesh", &currentPrim, primitiveNames, 8)) {
            SetPrimitive((PrimitiveType)currentPrim);
        }

        ImGui::Separator();
        ImGui::Text("Animation (Atlas)");
        ImGui::DragInt("Rows", (int*)&emitter_->atlasRows, 1, 1, 16);
        ImGui::DragInt("Cols", (int*)&emitter_->atlasCols, 1, 1, 16);

        ImGui::Separator();
        ImGui::Text("Physics Extension");
        ImGui::DragFloat("Ground Height", &emitter_->groundHeight, 0.1f, -100.0f, 100.0f);
        ImGui::DragFloat("Bounce", &emitter_->bounce, 0.01f, 0.0f, 1.0f);
        
        ImGui::DragFloat3("Attractor Pos", &emitter_->attractorPos.x, 0.1f);
        ImGui::DragFloat("Attractor Strength", &emitter_->attractorStrength, 0.1f, -100.0f, 100.0f);

        if (ImGui::Button("Burst (10)")) Emit(10);
    }
    ImGui::End();
#endif
}

void GPUParticleSystem::Clear() {
    isInitializedCS_ = false;
    totalTime_ = 0.0f;
}

void GPUParticleSystem::Emit(uint32_t count) {
    if (emitter_) {
        emitter_->burstCount += count;
    }
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

void GPUParticleSystem::SetEmit(bool emit) {
    if (emitter_) emitter_->emit = emit ? 1 : 0;
}

void GPUParticleSystem::SetPrimitive(PrimitiveType type) {
    primitiveType_ = type;
    const auto& res = PrimitiveManager::GetInstance()->GetStandardResource(type);
    vertexBufferView_ = res.vertexBufferView;
    indexBufferView_ = res.indexBufferView;
    indexCount_ = res.indexCount;
}

void GPUParticleSystem::SetBillboard(bool isBillboard) {
    if (emitter_) emitter_->isBillboard = isBillboard ? 1 : 0;
}

void GPUParticleSystem::SetRingEmitter(const Vector3& pos, float radius, float thickness, uint32_t count, float frequency) {
    if (!emitter_) return;
    emitter_->type = 2;
    emitter_->translate = pos;
    emitter_->radius = radius;
    emitter_->spread = thickness; // spreadをthicknessとして流用
    emitter_->count = (int32_t)count;
    emitter_->frequency = frequency;
}

void GPUParticleSystem::SetCylinderEmitter(const Vector3& pos, const Vector3& direction, float radius, float height, uint32_t count, float frequency) {
    if (!emitter_) return;
    emitter_->type = 3;
    emitter_->translate = pos;
    emitter_->direction = direction;
    emitter_->radius = radius;
    emitter_->velocity = height; // velocityをheightとして流用
    emitter_->count = (int32_t)count;
    emitter_->frequency = frequency;
}

void GPUParticleSystem::SetTextureAtlas(uint32_t rows, uint32_t cols) {
    if (emitter_) {
        emitter_->atlasRows = rows;
        emitter_->atlasCols = cols;
    }
}

void GPUParticleSystem::SetGroundCollision(float height, float bounce) {
    if (emitter_) {
        emitter_->groundHeight = height;
        emitter_->bounce = bounce;
    }
}

void GPUParticleSystem::SetAttractor(const Vector3& pos, float strength) {
    if (emitter_) {
        emitter_->attractorPos = pos;
        emitter_->attractorStrength = strength;
    }
}