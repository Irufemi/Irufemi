#pragma once

#include <d3d12.h>
#include <string>
#include <wrl.h>
#include <cstdint>

class DirectXCommon;
class DescriptorAllocator; // 追加

class Texture {
public:
    static void SetDirectXCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }
    static void SetDescriptorAllocator(DescriptorAllocator* alloc) { s_srvAllocator_ = alloc; }
    static DescriptorAllocator* GetDescriptorAllocator() { return s_srvAllocator_; }

    Texture() ;
    ~Texture();

    void Initialize(const std::string& filePath);

    const D3D12_GPU_DESCRIPTOR_HANDLE& GetTextureSrvHandleGPU()const { return textureSrvHandleGPU_; }

    // 追加: サイズ取得（TextureManager::GetTextureSize から呼ばれる）
    uint32_t GetWidth()  const { return width_; }
    uint32_t GetHeight() const { return height_; }

    // 既存互換（段階移行用）
    static uint32_t GetStaticSRVIndex() { return index_; }
    static void AddStaticSRVIndex() { index_++; }

protected:
    D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};
    std::string filePath_;
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource_ = nullptr;

    static uint32_t index_; // 互換用
    uint32_t srvIndex_ = UINT32_MAX; // allocator で確保した index

    static DirectXCommon* dxCommon_;
    static DescriptorAllocator* s_srvAllocator_;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

