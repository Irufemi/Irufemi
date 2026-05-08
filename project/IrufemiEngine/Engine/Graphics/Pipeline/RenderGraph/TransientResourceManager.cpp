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
    activeResources_.clear();
    heap_.Reset();
}

void TransientResourceManager::ResetForFrame() {
    // 前フレームで作成した PlacedResource はコマンド完了後に安全に破棄されるよう、
    // ここで dxCommon_->ReleaseAfterFence() のような仕組みを使うか、あるいは単にComPtrを手放す。
    // RenderGraph はフレームごとに再構築されるため、ひとまずクリアで参照カウントを減らす。
    // ※実運用では GPUが使用中かどうか Fence で待つか、ReleaseAfterFence キューに入れるのが安全。
    for (auto& res : activeResources_) {
        dxCommon_->ReleaseAfterFence(res);
    }
    activeResources_.clear();
}

ID3D12Resource* TransientResourceManager::AcquirePlacedResource(const D3D12_RESOURCE_DESC& desc, uint64_t offset, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue) {
    if (!heap_) return nullptr;

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
        activeResources_.push_back(resource);
        return resource.Get();
    }
    return nullptr;
}
