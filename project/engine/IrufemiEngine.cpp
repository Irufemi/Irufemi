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
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "3D/Region.h"
#include "3D/SphereRegion.h"
#include "3D/TetraRegion.h"
#include "audio/Bgm.h"
#include "audio/Se.h"
#include "source/Texture.h"

#include "scene/IScene.h"
#include "scene/title/TitleScene.h"
#include "scene/inGame/GameScene.h"
#include "scene/result/ResultScene.h"
#include "scene/SceneName.h"
#include "externals/imgui/imgui.h"

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
    // --- SRV デスクリプタアロケータを先に作る（DirectX 初期化直後） ---
    const uint32_t srvDescriptorInc = dxCommon_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    {
        ID3D12DescriptorHeap* srvHeap = dxCommon_->GetSrvDescriptorHeap();
        srvAllocator_ = std::make_unique<DescriptorAllocator>(srvHeap, srvDescriptorInc);

        // ImGui 等が先頭を使っているなら予約
        srvAllocator_->ReservePrefix(1);

        // 注入：Texture と SphereRegion（他クラスも同様に SetXXXAllocator を用意して注入）
        Texture::SetDescriptorAllocator(srvAllocator_.get());
        SphereRegion::SetSrvAllocator(srvAllocator_.get());
        Region::SetSrvAllocator(srvAllocator_.get());        
        TetraRegion::SetSrvAllocator(srvAllocator_.get());   
        ParticleClass::SetSrvAllocator(srvAllocator_.get()); 
    }

    // --- ここでテクスチャ管理を初期化（注入済みなので Texture::Initialize はアロケータ経由で確保します） ---
    textureManager = std::make_unique<TextureManager>();
    textureManager->Initialize(dxCommon_.get());
    textureManager->LoadAllFromFolder("resources/");

    // --- ロード後：もし ImGui など既存のSRVがあるなら走査して free-list を再構築する（任意だが推奨） ---
    {
        ID3D12DescriptorHeap* srvHeap = dxCommon_->GetSrvDescriptorHeap();
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
        // 先頭予約も used に
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

    // できるだけ早くシーンを破棄（中でGPUリソースを持っている可能性が高い）
    if (sceneManager_) {
        sceneManager_.reset();
    }

    // 入力系の解放
    if (inputManager_) {
        inputManager_.reset();
    }
    // サウンド
    if (audioManager_) {
        audioManager_->Finalize();
        audioManager_.reset();
    }
    // 描画
    if (drawManager) {
        drawManager->Finalize();
        drawManager.reset();
    }
    // UI
    if (ui) {
        ui->Shutdown();
        ui.reset();
    }
    // テクスチャ（SRV/テクスチャリソースを解放）
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

namespace {
    constexpr std::array<const char*, static_cast<size_t>(SceneName::CountOfSceneName)> kSceneLabels = {
        "Title", // SceneName::title
        "InGame", // SceneName::inGame
        "Result", // SceneName::result
    };
    static_assert(kSceneLabels.size() == static_cast<size_t>(SceneName::CountOfSceneName), "mismatch");
} // namespace

void IrufemiEngine::Execute() {

    // SceneManager 構築・登録

    sceneManager_ = std::make_unique<SceneManager>(this);     // ★エンジンを渡す
    g_SceneManager = sceneManager_.get();

    // シーンを登録

    sceneManager_->Register(SceneName::title, [] { return std::make_unique<TitleScene>(); });
    sceneManager_->Register(SceneName::inGame, [] { return std::make_unique<GameScene>(); });
    sceneManager_->Register(SceneName::result, [] { return std::make_unique<ResultScene>(); });

    // 初期シーン
    sceneManager_->ChangeTo(SceneName::title);

    while (winApp_->ProcessMessages()) {
        // 入力
        inputManager_->Update();
        // ImGui
        ui->FrameStart();

#if defined(_DEBUG) || defined(DEVELOPMENT)

        ui->FPSDebug();

        // 　シーン選択UI（Requestで要求を出す）
        ImGui::Begin("Scene Selector");
        int idx = static_cast<int>(g_SceneManager->GetCurrent());
        if (ImGui::Combo("Scene", &idx, kSceneLabels.data(), static_cast<int>(kSceneLabels.size()))) {
            g_SceneManager->Request(static_cast<SceneName>(idx)); // or ChangeTo(...)
        }
        ImGui::End();

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