#pragma once

#include <d3d12.h>
#include <string>
#include <wrl.h>
#include <cstdint>
#include "DirectXTex/DirectXTex.h"

class DirectXCommon;
class DescriptorPool;

class Texture {
public:
    static void SetDirectXCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }
    static void SetDescriptorPool(DescriptorPool* pool) { s_srvPool_ = pool; }
    static DescriptorPool* GetDescriptorPool() { return s_srvPool_; }

    Texture() ;
    ~Texture();

    void Initialize(const std::string& filePath);
    void InitializeFromMemory(const std::string& name, const uint32_t* pixels, uint32_t width, uint32_t height);

    const D3D12_GPU_DESCRIPTOR_HANDLE& GetTextureSrvHandleGPU()const { return textureSrvHandleGPU_; }

    // ScratchImageを取得
    const DirectX::ScratchImage* GetScratchImage() const { return &mipImages_; }

    // サイズ取得(TextureManager::GetTextureSize から呼ばれる)
    uint32_t GetWidth()  const { return width_; }
    uint32_t GetHeight() const { return height_; }

    // 既存互換(段階移行用)
    static uint32_t GetStaticSRVIndex() { return index_; }
    static void AddStaticSRVIndex() { index_++; }

protected:
    D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};
    std::string filePath_;
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource_ = nullptr;
    DirectX::ScratchImage mipImages_;

    static uint32_t index_; // 互換用
    uint32_t srvIndex_ = UINT32_MAX; // allocator で確保した index

    static DirectXCommon* dxCommon_;
    static DescriptorPool* s_srvPool_;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

