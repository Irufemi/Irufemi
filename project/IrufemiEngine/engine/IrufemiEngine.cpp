#include "IrufemiEngine.h"

#include "Core/Math/Random/Random.h"
#include "Core/Math/Geometry/Math.h"

#include <cassert>
#include <DbgHelp.h>
#include <cstdint>
#include <format>
#include <algorithm>
#include <string>

#include "Renderer/VertexData.h"
#include "Renderer/D3D12ResourceUtil.h"
#include "Renderer/Object2D/Sprite/Sprite.h"
#include "Renderer/Object2D/Primitive/Circle2D.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"
#include "Renderer/Object3D/AnimationModel/AnimationModel.h"
#include "Renderer/Object3D/Primitive/SphereClass.h"
#include "Renderer/Object3D/Primitive/TriangleClass.h"
#include "Renderer/Object3D/Primitive/CubeClass.h"
#include "Renderer/Object3D/Primitive/PlaneClass.h"
#include "Renderer/Object3D/Primitive/CylinderClass.h"
#include "Renderer/Particle/ParticleSystem.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"
#include "Renderer/VoxelParticle/VoxelParticleSystem.h"
#include "Renderer/Region/Region.h"
#include "Renderer/Region/Primitive/SphereRegion.h"
#include "Renderer/Region/Primitive/TetraRegion.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "Renderer/Skybox/Skybox.h"
#include "../Resource/Audio/Bgm.h"
#include "../Resource/Audio/Se.h"
#include "../Resource/Texture/Texture.h"
#include "Manager/DebugUI.h"

#include "Framework/IScene.h"

#pragma comment(lib,"Dbghelp.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxcompiler.lib")

//デストラクタ
IrufemiEngine::~IrufemiEngine() { Finalize(); }

// 初期化
void IrufemiEngine::Initialize(const std::wstring& title, const int32_t& clientWidth, const int32_t& clientHeight) {
    
    /*CrashHandler*/
    SetUnhandledExceptionFilter(WinApp::ExportDump);

    // 時間計測の開始
    startTime_ = std::chrono::steady_clock::now();
    lastFrameTime_ = startTime_;

    // WinApp をエンジン内で生成・初期化(COM 初期化もここで実施される)
    winApp_ = std::make_unique<WinApp>();
    if (!winApp_->Initialize(GetModuleHandle(nullptr), clientWidth, clientHeight, title.c_str())) {
        assert(false && "WinApp::Initialize failed");
        return;
    }

    // ログを出せるようにする
    log_ = std::make_unique<Log>();
    log_->Initialize();

    // 乱数エンジンのシードを設定
    Random::SeedEngine();

    // AudioManagerの生成・Media Foundationの初期化
    audioManager_ = std::make_unique<AudioManager>();
    audioManager_->StartUp();
    // AudioManagerの初期化
    audioManager_->Initialize();
    // "resources"フォルダから音声ファイルをすべてロード
    audioManager_->LoadAllSoundsFromFolder("resources/");
    Bgm::SetAudioManager(audioManager_.get());
    Se::SetAudioManager(audioManager_.get());

    // DirectX 基盤
    dxCommon_ = std::make_unique<DirectXCommon>();
    dxCommon_->SetLog(log_.get());
    dxCommon_->Initialize(winApp_->GetHwnd(), winApp_->GetClientWidth(), winApp_->GetClientHeight());

    D3D12ResourceUtil::SetDirectXCommon(dxCommon_.get());
    D3D12ResourceUtilParticle::SetDirectXCommon(dxCommon_.get());
    D3D12ResourceUtilLine::SetDirectXCommon(dxCommon_.get());
    ModelRegion::SetDirectXCommon(dxCommon_.get());
    SphereRegion::SetDirectXCommon(dxCommon_.get());
    TetraRegion::SetDirectXCommon(dxCommon_.get());
    Line3DRegion::SetDirectXCommon(dxCommon_.get());

    // SRV デスクリプタプール
    {
        DescriptorPool* srvPool = dxCommon_->GetSrvPool();

        // 注入
        Texture::SetDescriptorPool(srvPool);
        SphereRegion::SetSrvAllocator(srvPool);
        ModelRegion::SetSrvAllocator(srvPool);
        TetraRegion::SetSrvAllocator(srvPool);
        ParticleSystem::SetSrvPool(srvPool);
        Line3DRegion::SetSrvAllocator(srvPool);
    }

    // テクスチャ管理
    textureManager = std::make_unique<TextureManager>();
    textureManager->Initialize(dxCommon_.get());

#if defined(_DEBUG) || defined(DEVELOPMENT)
    textureManager->LoadAllFromFolder("resources/");
#endif

    // モデル管理
    modelManager_ = std::make_unique<ModelManager>();
    modelManager_->Initialize(dxCommon_.get(),textureManager.get()); // dxCommon を渡す
    ObjClass::SetModelManager(modelManager_.get());
    ModelRegion::SetModelManager(modelManager_.get()); // Regionにも設定

    // 既存SRVの走査で free-list 再構築
    {
        DescriptorPool* srvPool = dxCommon_->GetSrvPool();
        ID3D12DescriptorHeap* srvHeap = srvPool->GetHeap();
        const uint32_t inc = dxCommon_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        auto toIndex = [&](D3D12_GPU_DESCRIPTOR_HANDLE h)->uint32_t {
            if (h.ptr == 0) return DescriptorPool::kInvalid;
            const auto heapStart = srvHeap->GetGPUDescriptorHandleForHeapStart().ptr;
            const uint64_t diff = (h.ptr - heapStart);
            return static_cast<uint32_t>(diff / inc);
            };

        std::vector<uint32_t> used;
        // 白テクスチャ
        if (auto white = textureManager->GetWhiteTextureHandle(); white.ptr != 0) {
            if (auto idx = toIndex(white); idx != DescriptorPool::kInvalid) used.push_back(idx);
        }
        // テクスチャキャッシュ
        for (auto& name : textureManager->GetTextureNames()) {
            auto h = textureManager->GetTextureHandle(name);
            if (auto idx = toIndex(h); idx != DescriptorPool::kInvalid) used.push_back(idx);
        }
        for (uint32_t i = 0; i < srvPool->BaseIndex(); ++i) used.push_back(i);

        std::sort(used.begin(), used.end());
        used.erase(std::unique(used.begin(), used.end()), used.end());

        srvPool->RebuildFreeListExcept(used);
    }

    // 入力
    inputManager_ = std::make_unique<InputManager>();
    inputManager_->Initialize(winApp_->GetHwnd());
    winApp_->SetInputManager(inputManager_.get());

    // UI
    ui = std::make_unique <DebugUI>();
    ui->Initialize(winApp_->GetHwnd(), dxCommon_.get());
    Sprite::SetDebugUI(ui.get());
    Circle2D::SetDebugUI(ui.get());
    ObjClass::SetDebugUI(ui.get());
    SphereClass::SetDebugUI(ui.get());
    TriangleClass::SetDebugUI(ui.get());
    CubeClass::SetDebugUI(ui.get());
    PlaneClass::SetDebugUI(ui.get());
    CylinderClass::SetDebugUI(ui.get());
    ParticleSystem::SetDebugUI(ui.get());

    // 描画
    drawManager = std::make_unique<DrawManager>();
    drawManager->Initialize(dxCommon_.get());
    Sprite::SetDrawManager(drawManager.get());
    Circle2D::SetDrawManager(drawManager.get());
    ObjClass::SetDrawManager(drawManager.get());
    SphereClass::SetDrawManager(drawManager.get());
    TriangleClass::SetDrawManager(drawManager.get());
    CubeClass::SetDrawManager(drawManager.get());
    PlaneClass::SetDrawManager(drawManager.get());
    CylinderClass::SetDrawManager(drawManager.get());
    ModelRegion::SetDrawManager(drawManager.get());
    SphereRegion::SetDrawManager(drawManager.get());
    TetraRegion::SetDrawManager(drawManager.get());
    ParticleSystem::SetDrawManager(drawManager.get());
    GPUParticleSystem::SetDrawManager(drawManager.get());
    ParticleSystem::SetEngine(this);
    GPUParticleSystem::SetEngine(this);
    Line3DRegion::SetDrawManager(drawManager.get());

    // テクスチャ
    ui->SetTextureManager(textureManager.get());
    Sprite::SetTextureManager(textureManager.get());
    Circle2D::SetTextureManager(textureManager.get());
    ObjClass::SetTextureManager(textureManager.get());
    SphereClass::SetTextureManager(textureManager.get());
    TriangleClass::SetTextureManager(textureManager.get());
    CubeClass::SetTextureManager(textureManager.get());
    PlaneClass::SetTextureManager(textureManager.get());
    CylinderClass::SetTextureManager(textureManager.get());
    ModelRegion::SetTextureManager(textureManager.get());
    SphereRegion::SetTextureManager(textureManager.get());
    TetraRegion::SetTextureManager(textureManager.get());
    ParticleSystem::SetTextureManager(textureManager.get());
    GPUParticleSystem::SetTextureManager(textureManager.get());

    animationManager_ = std::make_unique<AnimationManager>();
    animationManager_->Initialize(dxCommon_.get());

    AnimationModel::SetIrufemiEngine(this);
    winApp_->SetInputManager(inputManager_.get());

    Skybox::SetEngine(this);
    GPUParticleSystem::SetDXCommon(dxCommon_.get());
    VoxelParticleSystem::SetEngine(this);

    // --- 全画面用 RenderTexture の初期化 ---
    mainRenderTexture_ = std::make_unique<RenderTexture>();
    mainRenderTexture_->Initialize(dxCommon_.get(), GetClientWidth(), GetClientHeight(), DXGI_FORMAT_R8G8B8A8_UNORM, { clearColor_[0], clearColor_[1], clearColor_[2], clearColor_[3] });

    // --- PostProcessManager の初期化 ---
    postProcessManager_ = std::make_unique<PostProcessManager>();
    postProcessManager_->Initialize(dxCommon_->GetDevice(), dxCommon_->GetRootSignature(), DXGI_FORMAT_R8G8B8A8_UNORM);
    postProcessManager_->InitializeBuffers(GetClientWidth(), GetClientHeight(), dxCommon_.get());

    // ノイズテクスチャのロードとハンドル設定
    postProcessManager_->SetDissolveNoiseHandle(0, textureManager->GetTextureHandle("resources/noise0.png"));
    postProcessManager_->SetDissolveNoiseHandle(1, textureManager->GetTextureHandle("resources/noise1.png"));

    // --- 深度バッファの SRV 作成とマネージャーへの設定 ---
    depthSrvIndex_ = dxCommon_->GetSrvPool()->Allocate();
    D3D12_GPU_DESCRIPTOR_HANDLE depthSrvHandleGPU = dxCommon_->GetSrvPool()->GetGPUHandle(depthSrvIndex_);

    D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc{};
    depthSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthSrvDesc.Texture2D.MipLevels = 1;
    dxCommon_->GetDevice()->CreateShaderResourceView(dxCommon_->GetDepthStencilResource(), &depthSrvDesc, dxCommon_->GetSrvPool()->GetCPUHandle(depthSrvIndex_));

    postProcessManager_->SetDepthSrvHandle(depthSrvHandleGPU);

    // WinAppに自身(Engine)のポインタを設定
    winApp_->SetEngine(this);
}

// クリアカラーを float 指定できる 初期化
void IrufemiEngine::Initialize(const std::wstring& title, const int32_t& clientWidth, const int32_t& clientHeight,
                               float r, float g, float b, float a) {
    clearColor_ = { r, g, b, a };
    // 既存の Initialize を呼ぶ(互換性維持)
    Initialize(title, clientWidth, clientHeight);
}

// クリアカラーを std::array 指定できる 初期化
void IrufemiEngine::Initialize(const std::wstring& title, const int32_t& clientWidth, const int32_t& clientHeight,
                               const std::array<float, 4>& clearColor) {
    clearColor_ = clearColor;
    // 既存の Initialize を呼ぶ(互換性維持)
    Initialize(title, clientWidth, clientHeight);
}

// 追加: Vector4 版 Initialize
void IrufemiEngine::Initialize(const std::wstring& title, const int32_t& clientWidth, const int32_t& clientHeight,
                               const Vector4& clearColor) {
    clearColor_ = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
    Initialize(title, clientWidth, clientHeight);
}

void IrufemiEngine::Finalize() {
    if (sceneManager_) {
        sceneManager_.reset();
    }
    if (inputManager_) {
        inputManager_.reset();
    }
    if (audioManager_) {
        audioManager_->Finalize();
        audioManager_.reset();
    }
    if (drawManager) {
        drawManager->Finalize();
        drawManager.reset();
    }
    if (ui) {
        ui->Shutdown();
        ui.reset();
    }
    if (textureManager) {
        textureManager.reset();
    }
    if (modelManager_) {
        modelManager_.reset();
    }
    if (dxCommon_) {
        dxCommon_->Finalize(); dxCommon_.reset();
    }
    if (winApp_) {
        winApp_.reset();
    }
}

void IrufemiEngine::Execute() {
    // SceneManager 構築(エンジンは所有のみ)
    sceneManager_ = std::make_unique<SceneManager>(this);

    // Application からの登録を反映
    if (sceneRegistrar_) {
        sceneRegistrar_(*sceneManager_);
    }

    // 初期シーンが指定されていれば遷移
    if (!initialSceneName_.empty()) {
        sceneManager_->ChangeTo(initialSceneName_);
    }

    while (winApp_->ProcessMessages()) {
        // フレーム開始時の時間更新
        StartFrame();

        // ImGui
        ui->FrameStart();

#ifdef USE_IMGUI
        ui->FPSDebug();
        ui->BeginEngineDebugWindow();
        ui->SceneSelectorTab(sceneManager_.get());
        ui->PostProcessTab(this);
        if (auto* scene = sceneManager_->GetCurrentScene()) {
            scene->DrawDebugTab();
        }
        ui->EndEngineDebugWindow();
#endif // USE_IMGUI


        // 更新
        sceneManager_->Update();
    totalTime_ += deltaTime_;
    postProcessManager_->Update(totalTime_);

    // インプットを更新
    inputManager_->Update();

        // フレーム途中処理
        ProcessFrame();

        // 描画
        sceneManager_->Draw();

        // 終了処理
        EndFrame();
    }
}

// フレーム開始処理
void IrufemiEngine::StartFrame() {
    // 時間の更新
    auto now = std::chrono::steady_clock::now();
    deltaTime_ = std::chrono::duration<float>(now - lastFrameTime_).count();
    totalTime_ = std::chrono::duration<float>(now - startTime_).count();
    lastFrameTime_ = now;
}

// フレーム途中処理
void IrufemiEngine::ProcessFrame() {
    // 描画処理に入る前にImGui::Renderを積む
    ui->QueueDrawCommands();
    
    // 1. バックバッファをクリア (念のため)
    drawManager->PreDraw(clearColor_, 1.0f, 0);

    // 2. メインの描画先を RenderTexture に切り替え、指定のクリアカラーでクリア
    drawManager->BeginRenderTexture(mainRenderTexture_.get(), Vector4{ clearColor_[0], clearColor_[1], clearColor_[2], clearColor_[3] });
}

// フレーム終了処理
void IrufemiEngine::EndFrame() {
    // 3. RenderTexture への描画を終了 (SRV状態へ遷移)
    drawManager->EndRenderTexture(mainRenderTexture_.get());

    // 4. 描画先をバックバッファに戻す
    drawManager->SetRenderTargetToBackBuffer(false);

    // 5. ポストプロセス描画の実行
    // Outline のための深度バッファ遷移 (スタック内のどこかに Outline があれば適用)
    const auto& activeModes = postProcessManager_->GetActiveModes();
    bool hasOutline = std::find(activeModes.begin(), activeModes.end(), PostProcessMode::DepthBasedOutline) != activeModes.end();

    if (hasOutline) {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = dxCommon_->GetDepthStencilResource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = 0; // 深度値のみをターゲットにする
        dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);
        
        // 逆投影行列の更新
        if (auto* cameraData = drawManager->GetCameraData()) {
            postProcessManager_->GetOutlineParams().projectionInverse = Math::Inverse(cameraData->projection);
        }
    }

    // マネージャーに描画を委譲
    postProcessManager_->Draw(dxCommon_->GetCommandList(), mainRenderTexture_.get(), dxCommon_->GetRtvHandles(dxCommon_->GetCurrentBackBufferIndex()));

    if (hasOutline) {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = dxCommon_->GetDepthStencilResource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        barrier.Transition.Subresource = 0; // 深度値のみを元に戻す
        dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);
    }

    // 描画後処理
    ui->QueuePostDrawCommands();
    drawManager->PostDraw();

    // 5) フレーム終端で遅延解放の回収(フェンス完了値を渡す)
    if (auto* srvPool = dxCommon_->GetSrvPool()) {
        const uint64_t completed = dxCommon_->GetFence()->GetCompletedValue();
        srvPool->GarbageCollect(completed);
    }
}

void IrufemiEngine::OnResize(int32_t width, int32_t height) {
    if (width <= 0 || height <= 0) return;

    // 1. スワップチェーン、深度バッファのリサイズ
    dxCommon_->ResizeSwapChain(width, height);

    // 2. メインレンダーテクスチャの再生成
    mainRenderTexture_->Initialize(dxCommon_.get(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM, { clearColor_[0], clearColor_[1], clearColor_[2], clearColor_[3] });
    postProcessManager_->InitializeBuffers(width, height, dxCommon_.get());

    // 3. 深度バッファの SRV 再作成 (既存のインデックスを再利用)
    if (depthSrvIndex_ != 0xFFFFFFFF) {
        D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc{};
        depthSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        depthSrvDesc.Texture2D.MipLevels = 1;
        dxCommon_->GetDevice()->CreateShaderResourceView(dxCommon_->GetDepthStencilResource(), &depthSrvDesc, dxCommon_->GetSrvPool()->GetCPUHandle(depthSrvIndex_));

        // ポストプロセスマネージャーに新しいSRVハンドルを設定
        postProcessManager_->SetDepthSrvHandle(dxCommon_->GetSrvPool()->GetGPUHandle(depthSrvIndex_));
    }
}

void IrufemiEngine::SetCursorLocked(bool lock) {
    if (winApp_) {
        winApp_->SetCursorLocked(lock);
    }
}

bool IrufemiEngine::IsCursorLocked() const {
    if (winApp_) {
        return winApp_->IsCursorLocked();
    }
    return false;
}

void IrufemiEngine::ApplyPSO() {
    auto* pso = GetPSOManager()->Get(currentBlend_, currentDepth_, currentCull_);
    assert(pso && "PSO is null. Check PSOManager::Initialize and shader blobs.");
    if (pso) { drawManager->BindPSO(pso); }
}

void IrufemiEngine::ApplyParticlePSO() {
    auto* pso = GetPSOManager()->GetParticle(currentBlend_, currentDepth_, currentCull_);
    assert(pso && "Particle PSO is null. Check particle shader setup.");
    if (pso) { drawManager->BindPSO(pso); }
}

void IrufemiEngine::ApplySpritePSO() {
    auto* pso = GetPSOManager()->GetSprite(currentBlend_, currentDepth_, currentCull_);
    if (pso) { drawManager->BindPSO(pso); }
}

void IrufemiEngine::ApplyRegionPSO() {
    auto* pso = GetPSOManager()->GetRegion(currentBlend_, currentDepth_, currentCull_);
    drawManager->BindPSO(pso);
}

void IrufemiEngine::ApplyByGeometryShaderPSO() {
    auto* pso = GetPSOManager()->GetByGeometryShader(currentBlend_, currentDepth_, currentCull_);
    assert(pso && "ByGeometryShader PSO is null. Check PSOManager::Initialize and shader blobs.");
    if (pso) { drawManager->BindPSO(pso); }
}

void IrufemiEngine::ApplyLinePSO() {
    auto* pso = GetPSOManager()->GetLine(currentBlend_, currentDepth_, currentCull_);
    assert(pso && "Line PSO is null. Check PSOManager::Initialize and shader blobs.");
    if (pso) { drawManager->BindPSO(pso); }
}

void IrufemiEngine::ApplyLineInstancedPSO() {
    auto* pso = GetPSOManager()->GetLineInstanced(currentBlend_, currentDepth_, currentCull_);
    assert(pso && "LineInstanced PSO is null. Check PSOManager::Initialize and shader blobs.");
    if (pso) { drawManager->BindPSO(pso); }
}

void IrufemiEngine::ApplySkinningPSO()
{
    auto* pso = GetPSOManager()->GetSkinning(currentBlend_, currentDepth_, currentCull_);
    assert(pso && "Skinning PSO is null. Check PSOManager::Initialize and shader blobs.");
    if (pso) { drawManager->BindPSO(pso); }
}

void IrufemiEngine::ApplySkyboxPSO()
{
    // Skyboxは内側から見るので、前面カリング
    auto* pso = GetPSOManager()->GetSkybox(PSOManager::CullMode::Front);
    assert(pso && "Skybox PSO is null. Check Skybox shader setup.");
    if (pso) { drawManager->BindPSO(pso); }
}

void IrufemiEngine::ApplyGpuParticlePSO() {
    auto* pso = GetPSOManager()->GetGpuParticle(currentBlend_, currentDepth_, currentCull_);
    assert(pso && "GpuParticle PSO is null. Check GpuParticle shader setup.");
    if (pso) { drawManager->BindPSO(pso); }
}