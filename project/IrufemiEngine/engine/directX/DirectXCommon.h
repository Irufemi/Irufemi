#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <memory>
#include <chrono>
#include <vector>

#include "DirectXTex/DirectXTex.h"
#include "PSOManager.h"
#include "DescriptorPool.h"

class Log;

/// <summary>
/// DirectX基盤
/// </summary>
class DirectXCommon {
public: // メンバ関数
	// コンストラクタ
	DirectXCommon() = default;
	//デストラクタ
	~DirectXCommon() = default;

	void Finalize();

	// 初期化
	void Initialize(HWND hwnd, int32_t w, int32_t h);

	void SetLog(Log* log) { log_ = log; }

	/*三角形の色を変えよう*/

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

	Microsoft::WRL::ComPtr<ID3D12Resource>  UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

	static DirectX::ScratchImage LoadTexture(const std::string& flilePath);

	// FPS固定初期化
	void InitializeFixFPS();
	// FPS固定更新
	void UpdateFixFPS();

public: // ゲッター

	ID3D12Device* GetDevice() { return device_.Get(); }
	ID3D12CommandQueue* GetCommandQueue() { return commandQueue_.Get(); }
	ID3D12CommandAllocator* GetCommandAllocator() { return commandAllocator_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() { return commandList_.Get(); }
	IDXGISwapChain4* GetSwapChain() { return swapChain_.Get(); }
	ID3D12Fence* GetFence() { return fence_.Get(); }
	HANDLE& GetFenceEvent() { return fenceEvent_; }
	ID3D12DescriptorHeap* GetSrvDescriptorHeap() { return srvPool_->GetHeap(); }
	ID3D12DescriptorHeap* GetDsvDescriptorHeap() { return dsvDescriptorHeap_.Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetRtvHandles(UINT index) { return rtvHandles_[index]; }
	ID3D12Resource* GetSwapChainResources(UINT index) { return swapChainResources_[index].Get(); }
	D3D12_VIEWPORT& GetViewport() { return viewport_; }
	D3D12_RENDER_TARGET_VIEW_DESC& GetRtvDesc() { return rtvDesc_; }
	D3D12_RECT& GetScissorRect() { return scissorRect_; }
	HWND GetHwnd() { return hwnd_; }
	DXGI_SWAP_CHAIN_DESC1& GetSwapChainDesc() { return swapChainDesc_; }
	ID3D12RootSignature* GetRootSignature() { return rootSignature_.Get(); }
	uint64_t& GetFenceValue() { return fenceValue_; }
	int32_t& GetClientWidth() { return clientWidth_; }
	int32_t& GetClientHeight() { return clientHeight_; }
	PSOManager* GetPSOManager() { return psoManager_.get(); }
	DescriptorPool* GetSrvPool() const { return srvPool_.get(); }

	// Compute Shader用
	ID3D12RootSignature* GetComputeRootSignature() const { return computeRootSignature_.Get(); }
	ID3D12PipelineState* GetSkinningComputePSO() const { return skinningComputePSO_.Get(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetRTVGPUDescriptorHandle(uint32_t index);

	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetDSVGPUDescriptorHandle(uint32_t index);

private:
	/*開発用のUIを出そう*/

	/// <summary>
	/// デスクリプタ生成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	/// <summary>
	/// 指定番号のCPUデスクリプタハンドルを取得する
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

	/// <summary>
	/// 指定番号のGPUデスクリプタハンドルを取得する
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

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
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;

	// --- SwapChain & Render Targets ---

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2] = { nullptr };
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2]{};

	// --- Descriptor Heaps ---

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_ = nullptr;
	std::unique_ptr<DescriptorPool> srvPool_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_ = nullptr;

	uint32_t descriptorSizeRTV{};
	uint32_t descriptorSizeDSV{};

	// --- Depth & Pipeline State ---

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

	// --- Synchronization --

	Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_ = nullptr;

	// Log(ポインタ参照)
	Log* log_ = nullptr;

	// PSO 管理インスタンス
	std::unique_ptr<PSOManager> psoManager_ = nullptr;

	// --- Compute Shader ---
	Microsoft::WRL::ComPtr<ID3D12RootSignature> computeRootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> skinningComputePSO_ = nullptr;

	// 記録時間(FPS固定用)
	std::chrono::steady_clock::time_point  reference_;
};

