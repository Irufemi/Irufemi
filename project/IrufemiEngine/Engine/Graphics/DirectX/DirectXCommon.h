#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <memory>
#include <chrono>
#include <vector>
#include <mutex>
#include <string>

#include "FrameRateController.h"
#include "ShaderCompiler.h"
#include "RootSignatureConfig.h"
#include <array>

#include "../../../../externals/DirectXTex/DirectXTex.h"
#include "../Pipeline/PSOManager.h"
#include "DescriptorPool.h"
#include "../../Core/Math/Vector4.h"

class Log;
class IrufemiEngine;

/**
 * @brief 同時実行フレーム数 (トリプルバッファリング)
 */
static const uint32_t kMaxFramesInFlight = 3;

/**
 * @class DirectXCommon
 * @brief DirectX 12 の基盤となる主要機能を管理するクラス
 * @details デバイス、コマンドキュー、スワップチェーン、デスクリプタヒープなどの初期化と管理を行います。
 */
class DirectXCommon {
public: // メンバ関数
	/**
	 * @brief コンストラクタ
	 */
	DirectXCommon() = default;

	/**
	 * @brief デストラクタ
	 */
	~DirectXCommon() = default;

	/**
	 * @brief 解放処理
	 */
	void Finalize();

	/**
	 * @brief 初期化
	 * @param[in] hwnd ウィンドウハンドル
	 * @param[in] w クライアント領域の幅
	 * @param[in] h クライアント領域の高さ
	 */
	void Initialize(HWND hwnd, int32_t w, int32_t h);

	/**
	 * @brief スワップチェーンのリサイズ
	 */
	void ResizeSwapChain(int32_t width, int32_t height);

	/**
	 * @brief ロガーの設定
	 */
	void SetLog(Log* log) { log_ = log; }

	/**
	 * @brief バッファリソースの生成
	 */
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

	/**
	 * @brief UAV用バッファリソースの生成
	 */
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVBufferResource(size_t sizeInBytes);

	/**
	 * @brief テクスチャデータのアップロード
	 */
	Microsoft::WRL::ComPtr<ID3D12Resource>  UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages);

	/**
	 * @brief テクスチャリソースの生成
	 */
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

	/**
	 * @brief テクスチャファイルの読み込み
	 * @param[in] filePath ファイルパス
	 * @return 読み込んだ画像データ
	 */
	static DirectX::ScratchImage LoadTexture(const std::string& filePath);

	/**
	 * @brief テクスチャメタデータの取得（ヘッダのみ読み込み）
	 * @param[in] filePath ファイルパス
	 * @return メタデータ
	 */
	static DirectX::TexMetadata GetTextureMetadata(const std::string& filePath);

	// ShaderCompilerに委譲したため、ここからは削除

	/**
	 * @brief 深度ステンシルテクスチャリソースの生成
	 */
	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height);

	/**
	 * @brief 現在のバックバッファインデックスの取得
	 */
	static UINT GetBackBufferIndex(const Microsoft::WRL::ComPtr<IDXGISwapChain4>& swapChain);

	/**
	 * @brief デスクリプタヒープの生成
	 */
	static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device>& device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	/**
	 * @brief レンダーターゲットテクスチャリソースの生成
	 */
	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4* clearColor);

	/**
	 * @brief FPS固定のための初期化
	 */
	void InitializeFixFPS() { fpsController_->Initialize(); }

	/**
	 * @brief FPS固定のための更新
	 */
	void UpdateFixFPS() { fpsController_->Update(); }
 
	/**
	 * @brief リソースの遅延解放登録
	 * @details GPUがリソースの使用を終えるまで解放を待機させます。
	 */
	void ReleaseAfterFence(Microsoft::WRL::ComPtr<ID3D12Resource> resource);

	/**
	 * @brief 待機中のリソースを解放する
	 */
	void ClearPendingResources();

	/**
	 * @brief エンジン本体へのポインタを設定
	 */
	void SetEngine(IrufemiEngine* engine) { engine_ = engine; }

	/**
	 * @brief エンジン本体へのポインタを取得
	 */
	IrufemiEngine* GetEngine() const { return engine_; }

public: // ゲッター

	/** @name D3D12 コアオブジェクトの取得 */
	///@{
	ID3D12Device* GetDevice() { return device_.Get(); }
	ID3D12CommandQueue* GetCommandQueue() { return commandQueue_.Get(); }
	ID3D12CommandAllocator* GetCommandAllocator() { return commandAllocators_[frameIndex_].Get(); }
	ID3D12GraphicsCommandList* GetCommandList() { return commandList_.Get(); }
	///@}

	/** @name スワップチェーン関連の取得 */
	///@{
	IDXGISwapChain4* GetSwapChain() { return swapChain_.Get(); }
	ID3D12Resource* GetSwapChainResources(UINT index) { return swapChainResources_[index].Get(); }
	UINT GetCurrentBackBufferIndex() const { return swapChain_->GetCurrentBackBufferIndex(); }
	D3D12_RENDER_TARGET_VIEW_DESC& GetRtvDesc() { return rtvDesc_; }
	///@}

	/** @name 同期・フェンス関連の取得 */
	///@{
	ID3D12Fence* GetFence() { return fence_.Get(); }
	HANDLE& GetFenceEvent() { return fenceEvent_; }
	uint64_t& GetFenceValue() { return fenceValues_[frameIndex_]; }
	uint64_t GetFenceValue(uint32_t index) const { return fenceValues_[index]; }
	///@}

	/** @name デスクリプタヒープ・ハンドルの取得 */
	///@{
	ID3D12DescriptorHeap* GetSrvDescriptorHeap() { return srvPool_->GetHeap(); }
	ID3D12DescriptorHeap* GetDsvDescriptorHeap() { return dsvDescriptorHeap_.Get(); }
	DescriptorPool* GetSrvPool() const { return srvPool_.get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetRtvHandles(UINT index) { return rtvHandles_[index]; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetRTVGPUDescriptorHandle(uint32_t index);
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetDSVGPUDescriptorHandle(uint32_t index);
	///@}

	/** @name ビューポート・矩形情報の取得 */
	///@{
	D3D12_VIEWPORT& GetViewport() { return viewport_; }
	D3D12_RECT& GetScissorRect() { return scissorRect_; }
	///@}

	/** @name その他情報の取得 */
	///@{
	HWND GetHwnd() { return hwnd_; }
	DXGI_SWAP_CHAIN_DESC1& GetSwapChainDesc() { return swapChainDesc_; }
	ID3D12RootSignature* GetRootSignature() { return rootSignature_.Get(); }
	int32_t& GetClientWidth() { return clientWidth_; }
	int32_t& GetClientHeight() { return clientHeight_; }
	PSOManager* GetPSOManager() { return psoManager_.get(); }
	ID3D12Resource* GetDepthStencilResource() const { return depthStencilResource_.Get(); }
	ShaderCompiler* GetShaderCompiler() const { return shaderCompiler_.get(); }
	FrameRateController* GetFPSController() const { return fpsController_.get(); }
	uint32_t GetFrameIndex() const { return frameIndex_; }
	void AdvanceFrameIndex() { frameIndex_ = (frameIndex_ + 1) % kMaxFramesInFlight; }
	///@}

	/** @name Compute Shader 関連の取得 */
	///@{
	ID3D12RootSignature* GetComputeRootSignature() const { return computeRootSignature_.Get(); }
	ID3D12PipelineState* GetSkinningComputePSO() const { return skinningComputePSO_.Get(); }
	ID3D12PipelineState* GetGpuParticleInitializePSO() const { return gpuParticleInitializePSO_.Get(); }
	ID3D12PipelineState* GetGpuParticleUpdatePSO() const { return gpuParticleUpdatePSO_.Get(); }
	ID3D12PipelineState* GetGpuParticleEmitPSO() const { return gpuParticleEmitPSO_.Get(); }
	ID3D12PipelineState* GetVoxelParticleInitializePSO() const { return voxelParticleInitializePSO_.Get(); }
	ID3D12PipelineState* GetVoxelParticleEmitPSO() const { return voxelParticleEmitPSO_.Get(); }
	ID3D12PipelineState* GetVoxelParticleUpdatePSO() const { return voxelParticleUpdatePSO_.Get(); }
	///@}

	/**
	 * @brief RTVインデックスの割り当て
	 */
	uint32_t AllocateRTVIndex();

	/**
	 * @brief DSVインデックスの割り当て
	 */
	uint32_t AllocateDSVIndex();

private:
	/**
	 * @brief デスクリプタヒープの生成（内部用）
	 */
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	void ReleaseSwapChainResources();

	/**
	 * @brief CPUデスクリプタハンドルの取得
	 */
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

	/**
	 * @brief GPUデスクリプタハンドルの取得
	 */
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

private: // 初期化用プライベートメソッド

	/** @name 初期化工程の分割 */
	///@{
	void EnableDebugLayer();
	void InitializeDXGI();
	void CreateDevice();
	void SetInfoQueue();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateDescriptorHeaps();
	void InitializeRenderTargets();
	void CreateDepthStencil();
	void CreateFence();
	void CreateRootSignatures();
	void CreatePSOs();
	///@}

private: // メンバ変数

	// --- Window ---

	HWND hwnd_{};
	// 画面横幅
	int32_t clientWidth_ = 1280;
	// 画面縦幅
	int32_t clientHeight_ = 720;

	//ビューポート
	D3D12_VIEWPORT viewport_ = D3D12_VIEWPORT{};
	//シザー矩形
	D3D12_RECT scissorRect_ = D3D12_RECT{};

	// --- D3D Device & Core ---

	//DXGIファクトリー
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators_[kMaxFramesInFlight];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;

	// --- SwapChain & Render Targets ---

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2] = { nullptr };
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[4]{};

	// --- Descriptor Heaps ---

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_ = nullptr;
	std::unique_ptr<DescriptorPool> srvPool_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_ = nullptr;

	uint32_t descriptorSizeRTV_ = 0;
	uint32_t descriptorSizeDSV_ = 0;
	uint32_t nextRtvIndex_ = 0;
	uint32_t nextDsvIndex_ = 0;

	// --- Depth & Pipeline State ---

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

	// --- Synchronization --

	Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
	uint64_t fenceValues_[kMaxFramesInFlight]{};
	HANDLE fenceEvent_ = nullptr;
	uint32_t frameIndex_ = 0;

	// Log(ポインタ参照)
	Log* log_ = nullptr;

	// PSO 管理インスタンス
	std::unique_ptr<PSOManager> psoManager_ = nullptr;

	// --- Compute Shader ---
	Microsoft::WRL::ComPtr<ID3D12RootSignature> computeRootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> skinningComputePSO_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> gpuParticleInitializePSO_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> gpuParticleUpdatePSO_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> gpuParticleEmitPSO_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> voxelParticleInitializePSO_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> voxelParticleEmitPSO_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> voxelParticleUpdatePSO_ = nullptr;

	// --- 制御用クラス ---
	std::unique_ptr<FrameRateController> fpsController_ = nullptr;
	std::unique_ptr<ShaderCompiler> shaderCompiler_ = nullptr;

	// --- リソース遅延解放用 ---
	struct PendingResource {
		uint64_t fenceValue;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	};
	std::vector<PendingResource> pendingResources_;

	// --- 非同期転送用 ---
	std::mutex uploadMutex_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> uploadCommandAllocator_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> uploadCommandList_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> uploadFence_ = nullptr;
	uint64_t uploadFenceValue_ = 0;

	// エンジン本体への参照
	IrufemiEngine* engine_ = nullptr;
};

