#include "TransientResourceManager.h"
#include "../../DirectX/DirectXCommon.h"
#include <stdexcept>

void TransientResourceManager::Initialize(DirectXCommon* dxCommon, uint64_t heapSizeInBytes) {
    dxCommon_ = dxCommon;
    heapSize_ = heapSizeInBytes;

    D3D12_HEAP_DESC heapDesc{};
    heapDesc.SizeInBytes = heapSize_;
    // テクスチャ（RT/DSV）やバッファの両方を配置可能にするアライメント設定
    heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapDesc.Properties.CreationNodeMask = 1;
    heapDesc.Properties.VisibleNodeMask = 1;
    heapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT; 
    heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

    HRESULT hr = dxCommon_->GetDevice()->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap_));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create Transient Resource Heap.");
    }
}

void TransientResourceManager::Finalize() {
    resourcePool_.clear();
    heap_.Reset();
}

void TransientResourceManager::ResetForFrame() {
    // キャッシュしたリソースを使用可能状態にリセットする
    for (auto& res : resourcePool_) {
        res.inUse = false;
    }
}

ID3D12Resource* TransientResourceManager::AcquirePlacedResource(const D3D12_RESOURCE_DESC& desc, uint64_t offset, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue) {
    if (!heap_) return nullptr;

    // 1. キャッシュから検索
    for (auto& res : resourcePool_) {
        bool clearValueMatch = false;
        if (!clearValue && !res.hasClearValue) clearValueMatch = true;
        else if (clearValue && res.hasClearValue && res.clearValue.Format == clearValue->Format) clearValueMatch = true; // 色の完全一致までは今回は省略可能

        if (!res.inUse && res.offset == offset &&
            res.desc.Width == desc.Width &&
            res.desc.Height == desc.Height &&
            res.desc.Format == desc.Format &&
            res.desc.Flags == desc.Flags &&
            res.desc.DepthOrArraySize == desc.DepthOrArraySize &&
            clearValueMatch) {
            
            res.inUse = true;
            return res.resource.Get();
        }
    }

    // 2. 見つからなければ新規作成
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    HRESULT hr = dxCommon_->GetDevice()->CreatePlacedResource(
        heap_.Get(),
        offset,
        &desc,
        initialState,
        clearValue,
        IID_PPV_ARGS(&resource)
    );

    if (SUCCEEDED(hr)) {
        CachedResource cache;
        cache.desc = desc;
        cache.offset = offset;
        cache.resource = resource;
        cache.inUse = true;
        resourcePool_.push_back(cache);
        return resource.Get();
    }
    
    // エラー時はログを出力
    char logMsg[256];
    sprintf_s(logMsg, "TransientResourceManager: CreatePlacedResource failed! Offset: %llu, HeapSize: %llu\n", offset, heapSize_);
    OutputDebugStringA(logMsg);
    
    return nullptr;
}
