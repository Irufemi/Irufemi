#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <memory>

class DescriptorAllocator;

// ディスクリプタハンドル（CPU/GPU/インデックス）をまとめた構造体
struct DescriptorHandle {
    D3D12_CPU_DESCRIPTOR_HANDLE cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu;
    uint32_t index;
};

class DescriptorPool {
public:
    DescriptorPool() = default;
    ~DescriptorPool() = default;

    // コピー禁止
    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;

    // 初期化
    void Initialize(
        ID3D12Device* device,
        uint32_t capacity,
        uint32_t reservedCount = 0);

    // ディスクリプタを確保
    DescriptorHandle Allocate();

    // ディスクリプタを即時解放
    void Free(uint32_t index);

    // ディスクリプタをフェンス完了後に解放
    void FreeAfterFence(uint32_t index, uint64_t fenceValue);

    // 指定フェンス値以下の保留中ディスクリプタを解放
    void GarbageCollect(uint64_t completedFence);

    // SRV生成: Texture2D
    DescriptorHandle CreateSRVForTexture2D(
        ID3D12Resource* resource,
        DXGI_FORMAT format,
        UINT mipLevels);

    // SRV生成: StructuredBuffer
    DescriptorHandle CreateSRVForStructuredBuffer(
        ID3D12Resource* resource,
        UINT numElements,
        UINT structureByteStride);

    // ヒープを取得
    ID3D12DescriptorHeap* GetHeap() const { return heap_.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
    std::unique_ptr<DescriptorAllocator> allocator_;
    uint32_t descriptorSize_ = 0;
};