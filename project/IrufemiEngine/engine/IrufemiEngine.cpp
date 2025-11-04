#include "IrufemiEngine.h"



#include "../function/Function.h"
#include "../function/GetBackBufferIndex.h"

#include <cassert>
#include <DbgHelp.h>
#include <cstdint>
#include <format>

#include "math/VertexData.h"
#include "source/D3D12ResourceUtil.h"
#include "2D/Sprite.h"
#include "2D/Circle2D.h"
#include "3D/ObjClass.h"
#include "3D/SphereClass.h"
#include "3D/TriangleClass.h"
#include "3D/CylinderClass.h"
#include "3D/ParticleClass.h"
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "3D/Region.h"
#include "3D/SphereRegion.h"
#include "3D/TetraRegion.h"
#include "audio/Bgm.h"
#include "audio/Se.h"
#include "source/Texture.h"

#include "scene/IScene.h"
#include <imgui.h>

#pragma comment(lib,"Dbghelp.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxcompiler.lib")

//デストラクタ
IrufemiEngine::~IrufemiEngine() { Finalize(); }

// 初期化
void IrufemiEngine::Initialize(const std::wstring& title, const int32_t& clientWidth, const int32_t& clientHeight) {
    /*CrashHandler*/
    SetUnhandledExceptionFilter(ExportDump);

    // WinApp をエンジン内で生成・初期化（COM 初期化もここで実施される）
    winApp_ = std::make_unique<WinApp>();
    if (!winApp_->Initialize(GetModuleHandle(nullptr), clientWidth, clientHeight, title.c_str())) {
        assert(false && "WinApp::Initialize failed");
        return;
    }

    // ログを出せるようにする
    log_ = std::make_unique<Log>();
    log_->Initialize();

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
    PointLightClass::SetDxCommon(dxCommon_.get());
    SpotLightClass::SetDxCommon(dxCommon_.get());
    Region::SetDirectXCommon(dxCommon_.get());
    SphereRegion::SetDirectXCommon(dxCommon_.get());
    TetraRegion::SetDirectXCommon(dxCommon_.get());

    // SRV デスクリプタアロケータの作成（3）
    const uint32_t srvDescriptorInc = dxCommon_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    {
        ID3D12DescriptorHeap* srvHeap = dxCommon_->GetSrvDescriptorHeap();
        srvAllocator_ = std::make_unique<DescriptorAllocator>(srvHeap, srvDescriptorInc);

        // ImGui 等が先頭を使っているなら予約
        srvAllocator_->ReservePrefix(1);

        // 注入
        Texture::SetDescriptorAllocator(srvAllocator_.get());
        SphereRegion::SetSrvAllocator(srvAllocator_.get());
        Region::SetSrvAllocator(srvAllocator_.get());
        TetraRegion::SetSrvAllocator(srvAllocator_.get());
        ParticleClass::SetSrvAllocator(srvAllocator_.get());
    }

    // テクスチャ管理
    textureManager = std::make_unique<TextureManager>();
    textureManager->Initialize(dxCommon_.get());
    textureManager->LoadAllFromFolder("resources/");

    // 既存SRVの走査で free-list 再構築
    {
        ID3D12DescriptorHeap* srvHeap = dxCommon_->GetSrvDescriptorHeap();
        const uint32_t inc = srvDescriptorInc;
        auto toIndex = [&](D3D12_GPU_DESCRIPTOR_HANDLE h)->uint32_t {
            if (h.ptr == 0) return DescriptorAllocator::kInvalid;
            const auto heapStart = srvHeap->GetGPUDescriptorHandleForHeapStart().ptr;
            const uint64_t diff = (h.ptr - heapStart);
            return static_cast<uint32_t>(diff / srvDescriptorInc);
            };

        std::vector<uint32_t> used;
        // 白テクスチャ
        if (auto white = textureManager->GetWhiteTextureHandle(); white.ptr != 0) {
            if (auto idx = toIndex(white); idx != DescriptorAllocator::kInvalid) used.push_back(idx);
        }
        // テクスチャキャッシュ
        for (auto& name : textureManager->GetTextureNames()) {
            auto h = textureManager->GetTextureHandle(name);
            if (auto idx = toIndex(h); idx != DescriptorAllocator::kInvalid) used.push_back(idx);
        }
        for (uint32_t i = 0; i < srvAllocator_->BaseIndex(); ++i) used.push_back(i);

        std::sort(used.begin(), used.end());
        used.erase(std::unique(used.begin(), used.end()), used.end());

        srvAllocator_->RebuildFreeListExcept(used);
    }

    // 入力
    inputManager_ = std::make_unique<InputManager>();
    inputManager_->Initialize();

    // UI
    ui = std::make_unique <DebugUI>();
    ui->Initialize(GetCommandList(), GetDevice(), GetHwnd(), GetSwapChainDesc(), GetRtvDesc(), GetSrvDescriptorHeap());
    Sprite::SetDebugUI(ui.get());
    Circle2D::SetDebugUI(ui.get());
    ObjClass::SetDebugUI(ui.get());
    SphereClass::SetDebugUI(ui.get());
    TriangleClass::SetDebugUI(ui.get());
    CylinderClass::SetDebugUI(ui.get());

    // 描画
    drawManager = std::make_unique<DrawManager>();
    drawManager->Initialize(dxCommon_.get());
    Sprite::SetDrawManager(drawManager.get());
    Circle2D::SetDrawManager(drawManager.get());
    ObjClass::SetDrawManager(drawManager.get());
    SphereClass::SetDrawManager(drawManager.get());
    TriangleClass::SetDrawManager(drawManager.get());
    CylinderClass::SetDrawManager(drawManager.get());
    Region::SetDrawManager(drawManager.get());
    SphereRegion::SetDrawManager(drawManager.get());
    TetraRegion::SetDrawManager(drawManager.get());

    // テクスチャ
    ui->SetTextureManager(textureManager.get());
    Sprite::SetTextureManager(textureManager.get());
    Circle2D::SetTextureManager(textureManager.get());
    ObjClass::SetTextureManager(textureManager.get());
    SphereClass::SetTextureManager(textureManager.get());
    TriangleClass::SetTextureManager(textureManager.get());
    CylinderClass::SetTextureManager(textureManager.get());
    Region::SetTextureManager(textureManager.get());
    SphereRegion::SetTextureManager(textureManager.get());
    TetraRegion::SetTextureManager(textureManager.get());
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
    if (dxCommon_) {
        dxCommon_->Finalize(); dxCommon_.reset();
    }
    if (winApp_) {
        winApp_.reset();
    }
}

void IrufemiEngine::Execute() {
    // SceneManager 構築（エンジンは所有のみ）
    sceneManager_ = std::make_unique<SceneManager>(this);
    // g_SceneManager = sceneManager_.get(); // グローバルは使用しない

    // Application からの登録を反映
    if (sceneRegistrar_) {
        sceneRegistrar_(*sceneManager_);
    }

    // 初期シーンが指定されていれば遷移
    if (!initialSceneName_.empty()) {
        sceneManager_->ChangeTo(initialSceneName_);
    }

    while (winApp_->ProcessMessages()) {
        // 入力
        inputManager_->Update();
        // ImGui
        ui->FrameStart();

#if defined(_DEBUG) || defined(DEVELOPMENT)
        ui->FPSDebug();
        ui->DebugSceneSelector(sceneManager_.get());
#endif // _DEBUG

        // 更新
        sceneManager_->Update();

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

}

// フレーム途中処理
void IrufemiEngine::ProcessFrame() {
    // 描画処理に入る前にImGui::Renderを積む
    ui->QueueDrawCommands();

    //これから書き込むバックバッファのインデックスを取得
    backBufferIndex_ = GetBackBufferIndex(GetSwapChain());

    ///DSVを設定する

    //描画先のRTVとDSVを設定する
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDsvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

    drawManager->PreDraw(clearColor_, 1.0f, 0);
}

// フレーム終了処理
void IrufemiEngine::EndFrame() {
    // 描画後処理
    ui->QueuePostDrawCommands();
    drawManager->PostDraw();

    // 5) フレーム終端で遅延解放の回収（フェンス完了値を渡す）
    if (srvAllocator_) {
        const uint64_t completed = dxCommon_->GetFence()->GetCompletedValue();
        srvAllocator_->GarbageCollect(completed);
    }
}

void IrufemiEngine::ApplyPSO() {
    auto* pso = GetPSOManager()->Get(currentBlend_, currentDepth_);
    assert(pso && "PSO is null. Check PSOManager::Initialize and shader blobs.");
    if (pso) { drawManager->BindPSO(pso); }
}

void IrufemiEngine::ApplyParticlePSO() {
    auto* pso = GetPSOManager()->GetParticle(currentBlend_, currentDepth_);
    assert(pso && "Particle PSO is null. Check particle shader setup.");
    if (pso) { drawManager->BindPSO(pso); }
}

void IrufemiEngine::ApplySpritePSO() {
    auto* pso = GetPSOManager()->GetSprite(currentBlend_, currentDepth_);
    if (pso) { drawManager->BindPSO(pso); }
}

void IrufemiEngine::ApplyRegionPSO() {
    auto* pso = GetPSOManager()->GetRegion(currentBlend_, currentDepth_);
    drawManager->BindPSO(pso);
}