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
#include "Engine/Manager/DebugUI.h"
#include "Renderer/LineInstanced/LineClass.h"
#include <cassert>
#include <vector>
#include <algorithm>

// 静的メンバ変数の実体定義
DirectXCommon* GPUParticleSystem::dxCommon_ = nullptr;
DrawManager* GPUParticleSystem::drawManager_ = nullptr;
TextureManager* GPUParticleSystem::textureManager_ = nullptr;
IrufemiEngine* GPUParticleSystem::engine_ = nullptr;

// コンストラクタ
GPUParticleSystem::GPUParticleSystem() = default;

GPUParticleSystem::~GPUParticleSystem() {
    if (dxCommon_) {
        uint64_t fv = dxCommon_->GetCurrentFrameFenceValue();
        if (auto* srvPool = dxCommon_->GetSrvPool()) {
            srvPool->FreeAfterFence(emitterSrvIndex_, fv);
            srvPool->FreeAfterFence(perFrameSrvIndex_, fv);
            srvPool->FreeAfterFence(particleUavIndex_, fv);
            srvPool->FreeAfterFence(particleSrvIndex_, fv);
            srvPool->FreeAfterFence(freeListIndexUavIndex_, fv);
            srvPool->FreeAfterFence(freeListUavIndex_, fv);
        }
        dxCommon_->ReleaseAfterFence(particleResource_);
        dxCommon_->ReleaseAfterFence(freeListIndexResource_);
        dxCommon_->ReleaseAfterFence(freeListResource_);
    }
}

// 初期化
void GPUParticleSystem::Initialize(Camera* camera, const std::string& textureName) {

    assert(dxCommon_);
    assert(drawManager_);
    assert(textureManager_);
    assert(engine_);

    camera_ = camera;

    CreateBuffersAndViews();

    // 各GPUParticleSystemインスタンスごとに異なるシードを持たせて、乱数系列が完全に被るのを防ぐ
    static uint32_t s_uniqueSeed = 0;
    emitter_->randomSeed = ++s_uniqueSeed;

    // 形状の初期設定 (デフォルトは Quad/Plane)
    SetPrimitive(PrimitiveType::Plane);

    if (textureManager_) {
        auto textureNames = textureManager_->GetTextureNamesForDebug();
        auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
        if (it != textureNames.end()) {
            selectedTextureIndex_ = static_cast<int>(std::distance(textureNames.begin(), it));
        }
    }
    textureHandle_ = textureManager_->GetTextureHandle(textureName);

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

#if defined(USE_IMGUI)
    debugLineRegion_ = std::make_unique<Line3DRegion>();
    debugLineRegion_->Initialize(camera);
#endif

    // ゲーム開始時（ローディング中）にCSを使ったバッファ初期化を済ませる
    if (dxCommon_) {
        // 同期待ちをさせるため、初回フレームでの遅延をなくす
        dxCommon_->ExecuteUploadCommands([this](ID3D12GraphicsCommandList* cmdList) {
            ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
            cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

            cmdList->SetComputeRootSignature(dxCommon_->GetComputeRootSignature());
            
            // 1. 初期化シェーダーの実行 (VRAM上のデータ構造を初期化)
            cmdList->SetPipelineState(dxCommon_->GetGpuParticleInitializePSO());
            cmdList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_);
            cmdList->SetComputeRootDescriptorTable(6, freeListIndexUavHandleGPU_);
            cmdList->SetComputeRootDescriptorTable(7, freeListUavHandleGPU_);

            cmdList->Dispatch((kMaxParticles + 1023) / 1024, 1, 1);

            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            barrier.UAV.pResource = nullptr;
            cmdList->ResourceBarrier(1, &barrier);
            
            // 2. Emit / Update シェーダーを空バインドして JIT 誘発
            // 実行はしない（Descriptor等も最低限のまま）
            cmdList->SetPipelineState(dxCommon_->GetGpuParticleEmitPSO());
            cmdList->SetPipelineState(dxCommon_->GetGpuParticleUpdatePSO());
        });
        isInitializedCS_ = true;
    } else {
        isInitializedCS_ = false;
    }
}

// 更新
void GPUParticleSystem::Update() {
    if (!emitter_ || !camera_) return;

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

    perFrameData_->time = engine_->GetGameTime();
    perFrameData_->deltaTime = engine_->GetGameDeltaTime();

    // 射出間隔を上回ったら射出予約に加算
    if (emitter_->emit) {
        while (emitter_->frequency <= emitter_->frequencyTime && emitter_->frequency > 0.0f) {
            emitter_->burstCount += (uint32_t)emitter_->count;
            emitter_->frequencyTime -= emitter_->frequency;
        }
    } else {
        emitter_->frequencyTime = 0.0f; // Emit停止中ならタイマーリセット
    }
    if (debugLineRegion_) {
        debugLineRegion_->Update();
    }

    needsUpdateCS_ = true;
    
    // エンジンにCompute Shaderの実行を予約する
    if (engine_ && engine_->GetDrawManager()) {
        engine_->GetDrawManager()->RegisterComputeTask(this);
    }
}

void GPUParticleSystem::SyncBeforeDraw() {
    uint32_t frameIndex = dxCommon_->GetFrameIndex();
    
    // PerViewはUpdateが呼ばれなくても毎フレーム必ず最新化する（ポーズ中のカメラ移動・マルチバッファ対策）
    if (camera_) {
        perViewBuffer_[frameIndex]->viewProjection = camera_->GetViewProjectionMatrix3D();
        Matrix4x4 backToFrontMatrix_ = Math::MakeRotateYMatrix(0.0f);
        Matrix4x4 billboardMatrix_ = Math::Multiply(backToFrontMatrix_, camera_->GetCameraMatrix());
        billboardMatrix_.m[3][0] = 0.0f;
        billboardMatrix_.m[3][1] = 0.0f;
        billboardMatrix_.m[3][2] = 0.0f;
        perViewBuffer_[frameIndex]->billboardMatrix = billboardMatrix_;
    }

    // 同一フレーム内で複数回呼び出された場合は無駄な転送を防ぐ
    // 特に、DispatchCompute後のburstCount=0の再転送（バグ）を防ぐ効果がある
    if (lastUpdateFrame_ == frameIndex) {
        return;
    }
    
    emitterBuffer_.Update(*emitter_, frameIndex);
    perFrameBuffer_.Update(*perFrameData_, frameIndex);
    materialBuffer_.Update(cpuMaterialData_, frameIndex);
    
    lastUpdateFrame_ = frameIndex;
}

void GPUParticleSystem::DispatchCompute() {
    if (isCulled_ || !needsUpdateCS_) return;

    uint32_t frameIndex = dxCommon_->GetFrameIndex();
    
    SyncBeforeDraw();
    
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
    DispatchComputeShaders(commandList);

    // 送信済みのバーストカウントをリセットする
    emitter_->burstCount = 0;
    needsUpdateCS_ = false;
}

// 描画
void GPUParticleSystem::Draw() {

    if (isCulled_) return;

    uint32_t frameIndex = dxCommon_->GetFrameIndex();

    // 描画パスによってカメラが変わる可能性があるため、毎回の描画で更新
    perViewBuffer_[frameIndex]->viewProjection = camera_->GetViewProjectionMatrix3D();
    Matrix4x4 backToFrontMatrix_ = Math::MakeRotateYMatrix(0.0f);
    Matrix4x4 billboardMatrix_ = Math::Multiply(backToFrontMatrix_, camera_->GetCameraMatrix());
    billboardMatrix_.m[3][0] = 0.0f;
    billboardMatrix_.m[3][1] = 0.0f;
    billboardMatrix_.m[3][2] = 0.0f;
    perViewBuffer_[frameIndex]->billboardMatrix = billboardMatrix_;

    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // Graphics Draw
    commandList->SetGraphicsRootSignature(dxCommon_->GetRootSignature());
    engine_->SetBlend(selectedBlend_);
    engine_->SetDepthWrite(selectedDepth_);
    engine_->SetCull(selectedCull_);
    engine_->ApplyGpuParticlePSO();
      
    drawManager_->SubmitGPUParticle(
        vertexBufferView_,
        indexBufferView_,
        indexCount_,
        materialBuffer_.GetGPUVirtualAddress(frameIndex),
        perViewBuffer_.GetGPUVirtualAddress(frameIndex),
        emitterBuffer_.GetGPUVirtualAddress(frameIndex),
        particleSrvHandleGPU_,
        textureHandle_,
        kMaxParticles,
        particleResource_.Get()
    );

#if USE_IMGUI
    if (debugLineRegion_) {
        debugLineRegion_->Draw();
    }
#endif
}

// デバッグ
void GPUParticleSystem::Debug() {
#if defined(USE_IMGUI)

    if (debugLineRegion_) {
        debugLineRegion_->ClearInstances();
    }

    if (showEmitterArea_ && emitter_) {
        Vector4 color = { 0.0f, 1.0f, 0.0f, 1.0f };
        if (emitter_->type == 0) {
            DrawSphereWireframe(emitter_->translate, emitter_->radius, color);
        } else if (emitter_->type == 1) {
            // Beamの簡易描画
            Vector3 top = emitter_->translate + Math::Normalize(emitter_->direction) * 50.0f; // 50mまで描画
            DrawCylinderWireframe(emitter_->translate, emitter_->direction, emitter_->radius, 50.0f, color);
        } else if (emitter_->type == 2) {
            // Ring (内側・外側の円、厚み（スプレッド）の表現は簡易化)
            DrawCircle(emitter_->translate, emitter_->radius, {0,1,0}, color);
            DrawCircle(emitter_->translate, emitter_->radius - emitter_->spread, {0,1,0}, color);
        } else if (emitter_->type == 3) {
            // Cylinder (velocity を height として使っている)
            DrawCylinderWireframe(emitter_->translate, emitter_->direction, emitter_->radius, emitter_->velocity, color);
        } else if (emitter_->type == 4) {
            // Box
            Vector3 minP = emitter_->translate - emitter_->areaSize * 0.5f;
            Vector3 maxP = emitter_->translate + emitter_->areaSize * 0.5f;
            DrawAABB(minP, maxP, color);
        }
    }

    ImGui::Begin("GPUParticleSystem");

    if (ImGui::BeginTabBar("VariablesTabBar")) {
        if (ImGui::BeginTabItem("General")) {
            DebugGeneralSettings();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Emitter")) {
            DebugEmitterSettings();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Shape & Texture")) {
            DebugShapeSettings();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Physics")) {
            DebugPhysicsSettings();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
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

void GPUParticleSystem::SetParticleScale(const Vector3& startMin, const Vector3& startMax, const Vector3& endMin, const Vector3& endMax) {
    if (emitter_) {
        emitter_->startScaleMin = startMin;
        emitter_->startScaleMax = startMax;
        emitter_->endScaleMin = endMin;
        emitter_->endScaleMax = endMax;
    }
}

void GPUParticleSystem::SetParticleLife(float minLife, float maxLife) {
    if (emitter_) {
        emitter_->minLife = minLife;
        emitter_->maxLife = maxLife;
    }
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

void GPUParticleSystem::SetTexture(const std::string& textureFilePath) {
    if (!textureManager_) return;
    
    // 無条件に GetTextureHandle を呼び出し、確実に読み込み＆ハンドル取得を行う
    textureHandle_ = textureManager_->GetTextureHandle(textureFilePath);
    
    // UIコンボボックス用のインデックス同期
    auto textureNames = textureManager_->GetTextureNamesForDebug();
    auto it = std::find(textureNames.begin(), textureNames.end(), textureFilePath);
    if (it != textureNames.end()) {
        selectedTextureIndex_ = static_cast<int>(std::distance(textureNames.begin(), it));
    }
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

void GPUParticleSystem::SetBoxEmitter(const Vector3& pos, const Vector3& size, uint32_t count, float frequency) {
    if (!emitter_) return;
    emitter_->type = 4;
    emitter_->translate = pos;
    emitter_->areaSize = size;
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

void GPUParticleSystem::DrawAABB(const Vector3& min, const Vector3& max, const Vector4& color) {
    if (!debugLineRegion_) return;
    
    Vector3 v[8] = {
        { min.x, min.y, min.z }, { max.x, min.y, min.z }, { min.x, max.y, min.z }, { max.x, max.y, min.z },
        { min.x, min.y, max.z }, { max.x, min.y, max.z }, { min.x, max.y, max.z }, { max.x, max.y, max.z }
    };

    debugLineRegion_->AddInstance(v[0], v[1], color); debugLineRegion_->AddInstance(v[1], v[3], color);
    debugLineRegion_->AddInstance(v[3], v[2], color); debugLineRegion_->AddInstance(v[2], v[0], color);
    debugLineRegion_->AddInstance(v[4], v[5], color); debugLineRegion_->AddInstance(v[5], v[7], color);
    debugLineRegion_->AddInstance(v[7], v[6], color); debugLineRegion_->AddInstance(v[6], v[4], color);
    debugLineRegion_->AddInstance(v[0], v[4], color); debugLineRegion_->AddInstance(v[1], v[5], color);
    debugLineRegion_->AddInstance(v[2], v[6], color); debugLineRegion_->AddInstance(v[3], v[7], color);
}

void GPUParticleSystem::DrawCircle(const Vector3& center, float radius, const Vector3& axis, const Vector4& color) {
    if (!debugLineRegion_) return;
    Vector3 up = Math::Normalize(axis);
    Vector3 right = Math::Normalize(std::abs(up.y) > 0.9f ? Math::Cross({1,0,0}, up) : Math::Cross({0,1,0}, up));
    Vector3 forward = Math::Cross(right, up);

    const int segments = 32;
    Vector3 prevPos = center + right * radius;
    for (int i = 1; i <= segments; ++i) {
        float angle = (float)i / segments * 3.141592f * 2.0f;
        Vector3 pos = center + (right * std::cos(angle) + forward * std::sin(angle)) * radius;
        debugLineRegion_->AddInstance(prevPos, pos, color);
        prevPos = pos;
    }
}

void GPUParticleSystem::DrawSphereWireframe(const Vector3& center, float radius, const Vector4& color) {
    DrawCircle(center, radius, {1,0,0}, color);
    DrawCircle(center, radius, {0,1,0}, color);
    DrawCircle(center, radius, {0,0,1}, color);
}

void GPUParticleSystem::DrawCylinderWireframe(const Vector3& center, const Vector3& direction, float radius, float height, const Vector4& color) {
    if (!debugLineRegion_) return;
    Vector3 dir = Math::Normalize(direction);
    Vector3 top = center + dir * height;
    
    DrawCircle(center, radius, dir, color);
    DrawCircle(top, radius, dir, color);
    
    Vector3 right = Math::Normalize(std::abs(dir.y) > 0.9f ? Math::Cross({1,0,0}, dir) : Math::Cross({0,1,0}, dir));
    Vector3 forward = Math::Cross(right, dir);
    
    for (int i = 0; i < 4; ++i) {
        float angle = (float)i / 4.0f * 3.141592f * 2.0f;
        Vector3 offset = (right * std::cos(angle) + forward * std::sin(angle)) * radius;
        debugLineRegion_->AddInstance(center + offset, top + offset, color);
    }
}

void GPUParticleSystem::DispatchComputeShaders(ID3D12GraphicsCommandList* commandList) {
    // 0. 未初期化の場合、CSでバッファを初期化する
    if (!isInitializedCS_) {
        // すでにInitialize()で実行済みのため本来はここに来ないが、フェールセーフとして残す
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
        commandList->SetComputeRootConstantBufferView(4, emitterBuffer_.GetGPUVirtualAddress(frameIndex));
        commandList->SetComputeRootConstantBufferView(5, perFrameBuffer_.GetGPUVirtualAddress(frameIndex));
        
        uint32_t emitCount = emitterBuffer_[frameIndex]->burstCount;
        if (emitCount > 0) {
            char debugMsg[256];
            sprintf_s(debugMsg, "[GPUParticleSystem::Dispatch] frameIndex=%d, emitCount=%d\n", frameIndex, emitCount);
            OutputDebugStringA(debugMsg);

            commandList->Dispatch((emitCount + 1023) / 1024, 1, 1);
        }

        commandList->ResourceBarrier(1, &uavBarrier);

        // Update
        commandList->SetPipelineState(dxCommon_->GetGpuParticleUpdatePSO());
        commandList->SetComputeRootDescriptorTable(3, particleUavHandleGPU_);
        commandList->SetComputeRootDescriptorTable(6, freeListIndexUavHandleGPU_);
        commandList->SetComputeRootDescriptorTable(7, freeListUavHandleGPU_);
        commandList->SetComputeRootConstantBufferView(4, emitterBuffer_.GetGPUVirtualAddress(frameIndex));
        commandList->SetComputeRootConstantBufferView(5, perFrameBuffer_.GetGPUVirtualAddress(frameIndex));
        commandList->Dispatch((kMaxParticles + 1023) / 1024, 1, 1);

        commandList->ResourceBarrier(1, &uavBarrier);

        needsUpdateCS_ = false;
        isCsDispatchedThisFrame_ = true;
    }
}

void GPUParticleSystem::DebugGeneralSettings() {
#if defined(USE_IMGUI)
    ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);
    ImGui::Checkbox("Show Emitter Area", &showEmitterArea_);
    
    if (ImGui::Button("Play")) Play(); ImGui::SameLine();
    if (ImGui::Button("Stop")) Stop(); ImGui::SameLine();
    if (ImGui::Button("Clear")) Clear();

    ImGui::Separator();
    DebugUI::DebugPsoSettings(&selectedBlend_, &selectedDepth_, &selectedCull_, "##GPUPso");
#endif
}

void GPUParticleSystem::DebugEmitterSettings() {
#if defined(USE_IMGUI)
    if (!emitter_) return;

    const char* typeNames[] = { "Sphere", "Beam", "Ring", "Cylinder", "Box" };
    int type = (int)emitter_->type;
    if (ImGui::Combo("Type", &type, typeNames, IM_ARRAYSIZE(typeNames))) {
        emitter_->type = (uint32_t)type;
        if (type == 4 && emitter_->areaSize.x == 0 && emitter_->areaSize.y == 0 && emitter_->areaSize.z == 0) {
            emitter_->areaSize = { 10.0f, 10.0f, 10.0f };
        }
    }

    ImGui::DragFloat3("Translate", &emitter_->translate.x, 0.1f);
    
    if (emitter_->type == 0 || emitter_->type == 2 || emitter_->type == 3) {
        ImGui::DragFloat("Radius", &emitter_->radius, 0.1f, 0.0f, 100.0f);
    }
    if (emitter_->type == 4) {
        ImGui::DragFloat3("Area Size", &emitter_->areaSize.x, 0.1f, 0.0f, 100.0f);
    }
    
    ImGui::Separator();
    ImGui::Text("Velocity & Direction");
    ImGui::DragFloat3("Direction", &emitter_->direction.x, 0.01f);
    ImGui::DragFloat("Velocity", &emitter_->velocity, 0.01f);
    ImGui::DragFloat("Spread (Radial)", &emitter_->spread, 0.01f, 0.0f, 5.0f);

    ImGui::Separator();
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

    if (ImGui::Button("Burst (10)")) Emit(10);
#endif
}

void GPUParticleSystem::DebugShapeSettings() {
#if defined(USE_IMGUI)
    if (!emitter_) return;

    ImGui::Text("Mesh Shape & Render");
    bool billboard = emitter_->isBillboard != 0;
    if (ImGui::Checkbox("Billboard", &billboard)) {
        SetBillboard(billboard);
    }

    const char* primitiveNames[] = { "Triangle", "Plane", "Cube", "Cylinder", "Sphere", "Tetra", "Circle", "Ring" };
    int currentPrim = (int)primitiveType_;
    if (ImGui::Combo("Particle Mesh", &currentPrim, primitiveNames, 8)) {
        SetPrimitive((PrimitiveType)currentPrim);
    }

    if (textureManager_ && !textureManager_->GetTextureNamesForDebug().empty()) {
        auto textureNames = textureManager_->GetTextureNamesForDebug();
        std::vector<const char*> namesCStr;
        for (const auto& name : textureNames) {
            namesCStr.push_back(name.c_str());
        }
        if (ImGui::Combo("Texture", &selectedTextureIndex_, namesCStr.data(), (int)namesCStr.size())) {
            SetTexture(textureNames[selectedTextureIndex_]);
        }
    }

    ImGui::Separator();
    ImGui::Text("Animation (Atlas)");
    ImGui::DragInt("Rows", (int*)&emitter_->atlasRows, 1, 1, 16);
    ImGui::DragInt("Cols", (int*)&emitter_->atlasCols, 1, 1, 16);
#endif
}

void GPUParticleSystem::DebugPhysicsSettings() {
#if defined(USE_IMGUI)
    if (!emitter_) return;

    ImGui::Text("Physics & Kinetics");
    ImGui::DragFloat("Gravity", &emitter_->gravity, 0.01f);
    ImGui::DragFloat("Damping", &emitter_->damping, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("Jitter", &emitter_->jitter, 0.001f, 0.0f, 0.5f);
    
    ImGui::Separator();
    ImGui::Text("Physics Extension (Collision & Attractor)");
    ImGui::DragFloat("Ground Height", &emitter_->groundHeight, 0.1f, -100.0f, 100.0f);
    ImGui::DragFloat("Bounce", &emitter_->bounce, 0.01f, 0.0f, 1.0f);
    
    ImGui::DragFloat3("Attractor Pos", &emitter_->attractorPos.x, 0.1f);
    ImGui::DragFloat("Attractor Strength", &emitter_->attractorStrength, 0.1f, -100.0f, 100.0f);
#endif
}

void GPUParticleSystem::CreateBuffersAndViews() {
    auto* srvPool = dxCommon_->GetSrvPool();

    /*Emitter と PerFrame の定数バッファ初期化*/
    emitterBuffer_.Initialize(dxCommon_);
    perFrameBuffer_.Initialize(dxCommon_);
    *emitter_ = GPUParticleEmitter(); // 初期化マスター

    // SRV (emitter / perFrame の SRV は未使用のため削除)

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
    perViewBuffer_.Initialize(dxCommon_);

    // Material用リソース
    materialBuffer_.Initialize(dxCommon_);
    for(uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        materialBuffer_[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        materialBuffer_[i]->uvTransform = Math::MakeIdentity4x4();
        materialBuffer_[i]->useClampSampler = 0;
    }
}