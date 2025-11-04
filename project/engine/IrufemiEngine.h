#pragma once

#include "D3DResourceLeakChecker.h"
#include "../engine/Input/InputManager.h"
#include "../manager/DrawManager.h"
#include "../manager/DebugUI.h"
#include "../manager/TextureManager.h"
#include "../manager/AudioManager.h"
#include "../math/BlendMode.h"
#include <memory>
#include "Log.h"
#include <Windows.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include "directX/DirectXCommon.h"
#include "../scene/SceneManager.h"
#include "WinApp/WinApp.h"
#include "DescriptorAllocator.h" // 追加

class IrufemiEngine {

public:
    // 状態（現在のブレンドと深度書き込み）
    BlendMode currentBlend_ = BlendMode::kBlendModeNormal;               // 既定：通常α
    PSOManager::DepthWrite currentDepth_ = PSOManager::DepthWrite::Enable; // 既定：深度Write無効（透過系）

private: // メンバ変数

    // --- Debug & Logging ---

    // リソース解放リークチェック
    D3DResourceLeakChecker leakCheck_{};


    // ログ
    std::unique_ptr<Log> log_ = nullptr;

    // WinApp
    std::unique_ptr<WinApp> winApp_ = nullptr;

    // DirectX基盤
    std::unique_ptr<DirectXCommon> dxCommon_ = nullptr;

    // --- Manager ---

    // InputManager
    std::unique_ptr <InputManager> inputManager_ = nullptr;

    // DrawManager
    std::unique_ptr <DrawManager> drawManager = nullptr;

    // DebugUI
    std::unique_ptr <DebugUI> ui = nullptr;

    // TextureManager
    std::unique_ptr <TextureManager> textureManager = nullptr;

    // AudioManager
    std::unique_ptr<AudioManager> audioManager_ = nullptr;

    // ★SceneManager をエンジン内に保持
    std::unique_ptr<SceneManager> sceneManager_ = nullptr;

    // DescriptorAllocator
    std::unique_ptr<DescriptorAllocator> srvAllocator_; // 追加

    //画面の色
    std::array<float, 4> clearColor_{ 0.8f, 0.8f, 0.8f, 1.0f };

    //バックバッファのインデックス
    UINT backBufferIndex_{};

public: // メンバ関数
    // コンストラクタ
    IrufemiEngine() = default;

    //デストラクタ
    ~IrufemiEngine();

    // ループ丸ごと実行
    void Execute();

    /// <summary>
    ///  初期化
    /// </summary>
    void Initialize(const std::wstring& title, const int32_t& clientWidth = 1280, const int32_t& clientHeight = 720);

    /// <summary>
    /// 解放
    /// </summary>
    void Finalize();

    /// <summary>
    /// フレーム開始処理
    /// </summary>
    void StartFrame();

    /// <summary>
    /// フレーム途中処理
    /// </summary>
    void ProcessFrame();

    /// <summary>
    /// フレーム終了処理
    /// </summary>
    void EndFrame();

public: // ゲッター

    ID3D12GraphicsCommandList* GetCommandList() { return dxCommon_->GetCommandList(); }
    ID3D12Device* GetDevice() { return dxCommon_->GetDevice(); }
    HWND& GetHwnd() { return dxCommon_->GetHwnd(); }
    DXGI_SWAP_CHAIN_DESC1& GetSwapChainDesc() { return dxCommon_->GetSwapChainDesc(); }
    D3D12_RENDER_TARGET_VIEW_DESC& GetRtvDesc() { return dxCommon_->GetRtvDesc(); }
    ID3D12DescriptorHeap* GetSrvDescriptorHeap() { return dxCommon_->GetSrvDescriptorHeap(); }
    ID3D12CommandQueue* GetCommandQueue() { return dxCommon_->GetCommandQueue(); }
    IDXGISwapChain4* GetSwapChain() { return dxCommon_->GetSwapChain(); }
    ID3D12Fence* GetFence() { return dxCommon_->GetFence(); }
    HANDLE& GetFenceEvent() { return dxCommon_->GetFenceEvent(); }
    ID3D12CommandAllocator* GetCommandAllocator() { return dxCommon_->GetCommandAllocator(); }
    ID3D12RootSignature* GetRootSignature() { return dxCommon_->GetRootSignature(); }
    ID3D12DescriptorHeap* GetDsvDescriptorHeap() { return dxCommon_->GetDsvDescriptorHeap(); }
    ID3D12Resource* GetSwapChainResources(UINT index) { return dxCommon_->GetSwapChainResources(index); }
    D3D12_CPU_DESCRIPTOR_HANDLE& GetRtvHandles(UINT index) { return dxCommon_->GetRtvHandles(index); }
    uint64_t& GetFenceValue() { return dxCommon_->GetFenceValue(); }
    InputManager* GetInputManager() { return this->inputManager_.get(); }
    DrawManager* GetDrawManager() { return this->drawManager.get(); }
    DebugUI* GetDebugUI() { return this->ui.get(); }
    AudioManager* GetAudioManager() { return this->audioManager_.get(); }
    TextureManager* GetTextureManager() { return this->textureManager.get(); }
    int32_t& GetClientWidth() { return dxCommon_->GetClientWidth(); }
    int32_t& GetClientHeight() { return dxCommon_->GetClientHeight(); }
    D3D12_VIEWPORT& GetViewport() { return dxCommon_->GetViewport(); };
    D3D12_RECT& GetScissorRect() { return dxCommon_->GetScissorRect(); };
    PSOManager* GetPSOManager() { return dxCommon_->GetPSOManager(); }

    // オプション: 取得用
    DescriptorAllocator* GetSrvAllocator() const { return srvAllocator_.get(); }

public: // セッター
    void AddFenceValue(uint32_t index) { dxCommon_->GetFenceValue() += index; }

    // セッター（引数なし描画のためのプリセット切替）
    void SetBlend(BlendMode m) { currentBlend_ = m; }
    void SetDepthWrite(PSOManager::DepthWrite w) { currentDepth_ = w; }

    // 状態からPSOを適用してBind（引数なしで使うやつ）
    void ApplyPSO();
    void ApplyParticlePSO();
    void ApplySpritePSO();
    void ApplyRegionPSO();
};

