#pragma once

#include "Graphics/DirectX/DirectXCommon.h"
#include "Graphics/DirectX/D3DResourceLeakChecker.h"
#include "Platform/Input/InputManager.h"
#include "Platform/WindowsAPI/WinApp.h"
#include "Manager/DrawManager.h"
#include "Manager/DebugUI.h"
#include "Resource/Texture/TextureManager.h"
#include "Resource/Audio/AudioManager.h"
#include "Resource/Model/ModelManager.h"
#include "Resource/Model/AnimationManager.h"
#include "Core/Type/BlendMode.h"
#include "Core/Utility/Log.h"
#include "Framework/SceneManager.h"
#include "Engine/Core/Math/Vector4.h"
#include <memory>
#include <Windows.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <functional>
#include <string>
#include <array>

class SceneManager;
class DebugUI;

class IrufemiEngine {
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
   
    /// 追加: クリアカラーを引数で指定できる Initialize(float RGBA)
   void Initialize(const std::wstring& title, const int32_t& clientWidth, const int32_t& clientHeight,
                    float r, float g, float b, float a = 1.0f);
    
    /// 追加: クリアカラーを引数で指定できる Initialize(std::array)
    void Initialize(const std::wstring& title, const int32_t& clientWidth, const int32_t& clientHeight,
                    const std::array<float, 4>& clearColor);
    // 追加: Vector4 版
    void Initialize(const std::wstring& title, const int32_t& clientWidth, const int32_t& clientHeight,
                    const Vector4& clearColor);

     // --- Application からの注入用コールバック型とセッター ---
    using SceneRegistrar = std::function<void(SceneManager&)>;
   
    void SetSceneRegistrar(SceneRegistrar registrar) { sceneRegistrar_ = std::move(registrar); }
    void SetInitialSceneName(std::string name) { initialSceneName_ = std::move(name); }

 private: // メンバ関数(内部処理)

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
    HWND GetHwnd() { return dxCommon_->GetHwnd(); }
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
    DirectXCommon* GetDirectXCommon() { return this->dxCommon_.get(); }
    InputManager* GetInputManager() { return this->inputManager_.get(); }
    DrawManager* GetDrawManager() { return this->drawManager.get(); }
    DebugUI* GetDebugUI() { return this->ui.get(); }
    AudioManager* GetAudioManager() { return this->audioManager_.get(); }
    TextureManager* GetTextureManager() { return this->textureManager.get(); }
    ModelManager* GetObjModelManager() { return modelManager_.get(); }
    AnimationManager* GetAnimationManager() { return animationManager_.get(); }
    int32_t& GetClientWidth() { return dxCommon_->GetClientWidth(); }
    int32_t& GetClientHeight() { return dxCommon_->GetClientHeight(); }
    D3D12_VIEWPORT& GetViewport() { return dxCommon_->GetViewport(); }
    D3D12_RECT& GetScissorRect() { return dxCommon_->GetScissorRect(); }
    PSOManager* GetPSOManager() { return dxCommon_->GetPSOManager(); }

    // 時間関連のゲッター
    float GetDeltaTime() const { return deltaTime_; }
    float GetTotalTime() const { return totalTime_; }

    // オプション: 取得用
    DescriptorPool* GetSrvPool() const { return dxCommon_->GetSrvPool(); }
    
    // SceneManager参照
    SceneManager* GetSceneManager() const { return sceneManager_.get(); }

public: // セッター
    void AddFenceValue(uint32_t index) { dxCommon_->GetFenceValue() += index; }
    
    // セッター(引数なし描画のためのプリセット切替)
    void SetBlend(BlendMode m) { currentBlend_ = m; }
    void SetDepthWrite(PSOManager::DepthWrite w) { currentDepth_ = w; }
    // 追加: Cull の切替
    void SetCull(PSOManager::CullMode c) { currentCull_ = c; }

    // 追加: クリアカラーのセッター(いつでも変更可能)
    void SetClearColor(float r, float g, float b, float a = 1.0f) { clearColor_ = { r, g, b, a }; }
    void SetClearColor(const std::array<float, 4>& c) { clearColor_ = c; }
    // 追加: Vector4 版
    void SetClearColor(const Vector4& c) { clearColor_ = { c.x, c.y, c.z, c.w }; }

    // 状態からPSOを適用してBind(引数なしで使うやつ)
    void ApplyPSO();
    void ApplyParticlePSO();
    void ApplySpritePSO();
    void ApplyRegionPSO();
    void ApplyByGeometryShaderPSO();
    void ApplyLinePSO();
    void ApplyLineInstancedPSO();
    void ApplySkinningPSO();
    void ApplySkyboxPSO();
    void ApplyGpuParticlePSO();

public:
    // 状態(現在のブレンドと深度書き込み)
    BlendMode currentBlend_ = BlendMode::kBlendModeNormal;
    PSOManager::DepthWrite currentDepth_ = PSOManager::DepthWrite::Enable;
    PSOManager::CullMode currentCull_ = PSOManager::CullMode::Back; // 追加: デフォルトは Back

private: // メンバ変数

    // --- Debug & Logging ---

    // リソース解放リークチェック
    D3DResourceLeakChecker leakCheck_;
    
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
    
    // SceneManager
    std::unique_ptr<SceneManager> sceneManager_ = nullptr;
    
    // ModelManager
    std::unique_ptr<ModelManager> modelManager_ = nullptr;

    // AnimationManager
    std::unique_ptr<AnimationManager> animationManager_ = nullptr;

    //画面の色
    std::array<float, 4> clearColor_{ 0.1f, 0.25f, 0.5f, 1.0f };

    //バックバッファのインデックス
    UINT backBufferIndex_{};
    
    // Application から注入
    SceneRegistrar sceneRegistrar_{};
    std::string initialSceneName_{};

    // --- 時間管理 ---
    std::chrono::steady_clock::time_point startTime_{};
    std::chrono::steady_clock::time_point lastFrameTime_{};
    float deltaTime_ = 0.0f;
    float totalTime_ = 0.0f;
};

