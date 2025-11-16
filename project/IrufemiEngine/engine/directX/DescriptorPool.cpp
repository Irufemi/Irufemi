#include "DescriptorPool.h"
#include "DescriptorAllocator.h"
#include "../../function/Function.h" // CreateDescriptorHeap のため
#include <cassert>

void DescriptorPool::Initialize(
    ID3D12Device* device,
    uint32_t capacity,
    uint32_t reservedCount) {

    device_ = device;

    // ディスクリプタヒープを生成
    heap_ = CreateDescriptorHeap(
        device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        capacity,
        true);
    assert(heap_);

    // ディスクリプタサイズを取得
    descriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // アロケータを生成
    allocator_ = std::make_unique<DescriptorAllocator>(heap_.Get(), descriptorSize_);

    // 先頭を予約
    if (reservedCount > 0) {
        allocator_->ReservePrefix(reservedCount);
    }
}

DescriptorHandle DescriptorPool::Allocate() {
    assert(allocator_ && "DescriptorPool is not initialized.");
    uint32_t index = allocator_->Allocate();
    if (index == DescriptorAllocator::kInvalid) {
        // 空きがない場合、無効なハンドルを返す
        return { {0}, {0}, DescriptorAllocator::kInvalid };
    }
    return {
        allocator_->GetCPUHandle(index),
        allocator_->GetGPUHandle(index),
        index
    };
}

void DescriptorPool::Free(uint32_t index) {
    if (!allocator_) return;
    allocator_->Free(index);
}

void DescriptorPool::FreeAfterFence(uint32_t index, uint64_t fenceValue) {
    if (!allocator_) return;
    allocator_->FreeAfterFence(index, fenceValue);
}

void DescriptorPool::GarbageCollect(uint64_t completedFence) {
    if (!allocator_) return;
    allocator_->GarbageCollect(completedFence);
}

DescriptorHandle DescriptorPool::CreateSRVForTexture2D(
    ID3D12Resource* resource,
    DXGI_FORMAT format,
    UINT mipLevels) {

    DescriptorHandle handle = Allocate();
    if (handle.index == DescriptorAllocator::kInvalid) {
        return handle; // 確保失敗
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = mipLevels;

    device_->CreateShaderResourceView(resource, &srvDesc, handle.cpu);

    return handle;
}

DescriptorHandle DescriptorPool::CreateSRVForStructuredBuffer(
    ID3D12Resource* resource,
    UINT numElements,
    UINT structureByteStride) {

    DescriptorHandle handle = Allocate();
    if (handle.index == DescriptorAllocator::kInvalid) {
        return handle; // 確保失敗
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = numElements;
    srvDesc.Buffer.StructureByteStride = structureByteStride;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    device_->CreateShaderResourceView(resource, &srvDesc, handle.cpu);

    return handle;
}